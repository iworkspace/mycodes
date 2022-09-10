#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <poll.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <linux/if.h>
#include <linux/if_packet.h>
#include <linux/filter.h>
#include <linux/if_ether.h>
#include <linux/ip.h>


int sock_g = -1;
int loglevel = 0;

int usage(char *prog,int ret){
  printf("%s [ -i dev ] [ -b bpf_rule_file ] [ -l loglevel ] \n",prog);
  return ret;
}

int capture_packet_on_dev(char *ifname)
{
  struct ifreq ifr = {};
  struct sockaddr_ll addr = {};
  addr.sll_family = AF_PACKET;
  addr.sll_protocol = htons(ETH_P_ALL);
  addr.sll_ifindex=if_nametoindex(ifname);

  if(addr.sll_ifindex < 0) {
    fprintf(stderr,"ifnametoindex err\n");
    return -1;
  }

  if(bind(sock_g,(struct sockaddr*)&addr,sizeof(addr))){
    fprintf(stderr,"bind err\n");
    return -1;
  }

  //enable multicast & promisc
  strncpy(ifr.ifr_name,ifname,IFNAMSIZ);
  ioctl(sock_g,SIOCGIFFLAGS,&ifr);
  ifr.ifr_flags |= IFF_ALLMULTI|IFF_PROMISC;
  ioctl(sock_g,SIOCSIFFLAGS,&ifr);

  return 0;
}

/**
 *
 *
generating simple bpf bits file by tcpdump 
[test]# tcpdump -ddd -nn -i any ip
4
40 0 0 14
21 0 1 2048
6 0 0 262144
6 0 0 0
[test]#
[test]# tcpdump -dd -nn -i any ip
{ 0x28, 0, 0, 0x0000000e },
{ 0x15, 0, 1, 0x00000800 },
{ 0x6, 0, 0, 0x00040000 },
{ 0x6, 0, 0, 0x00000000 },
*/
int apply_bpf_rule(char *bpf_file){
  struct sock_fprog fprog;
  struct sock_filter *filter = NULL;
  int ret = -1;
  FILE *bpf_f= fopen(bpf_file,"rb+");
  if(!bpf_f){
    goto end;
  }

  if(1!=fscanf(bpf_f,"%d\n",&fprog.len)){
    goto end;
  }

  filter = malloc(sizeof(*filter)*(fprog.len+1));
  if(!filter){
    goto end;
  }

  int i;
  for(i=0;i<fprog.len;i++){
    if( 4 != fscanf(bpf_f,"%d %d %d %d\n",
          &filter[i].code,
          &filter[i].jt,
          &filter[i].jf,
          &filter[i].k)){
        goto end;
    }
  }
  fprog.filter = filter;

  if(setsockopt(sock_g,SOL_SOCKET,SO_ATTACH_FILTER,&fprog,sizeof(fprog))){
    goto end;
  }

  ret = 0;

end:
  if(filter){
      free(filter);
  }
  if(bpf_f){
    fclose(bpf_f);
  }
  return ret;
}

int  raw_socket_zero_copy_setup(struct iovec **ring,int *ring_size,int max_frame)
{
  //use zero copy raw packet walk...
  void *ring_map_addr;
  struct tpacket_req3 req = {};
  int i;
  int val = TPACKET_V3;
  int pagesize;
  int blocksize;
  int block_nr;
  int framesize;
  int frame_nr;
  int frame_nr_on_block;

  if(setsockopt(sock_g,SOL_PACKET,PACKET_VERSION,&val,sizeof(val))){
    fprintf(stderr,"set packet version 2 err\n");
    return -1;
  }

  /*
   *   
   tp_block_size must be a multiple of PAGE_SIZE (1)
   tp_frame_size must be greater than TPACKET_HDRLEN (obvious)
   tp_frame_size must be a multiple of TPACKET_ALIGNMENT
   tp_frame_nr   must be exactly frames_per_block*tp_block_nr
   * */
  pagesize = getpagesize();

  framesize = TPACKET_ALIGN(max_frame);
  blocksize = (framesize+pagesize-1)&(~(pagesize-1));
  frame_nr_on_block = blocksize/framesize;
  frame_nr = (*ring_size + frame_nr_on_block -1) & (~(frame_nr_on_block-1));
  block_nr = frame_nr/frame_nr_on_block;
  *ring_size = block_nr;

  req.tp_block_size = blocksize ; /* PAGE_ALIGNED > frame size*/
  req.tp_frame_size = framesize ; /* TAPCKET_ALIGNMENT  */
  req.tp_block_nr = block_nr ;
  req.tp_frame_nr = frame_nr ;

  if(setsockopt(sock_g,SOL_PACKET,PACKET_RX_RING,&req,sizeof(req))){
    fprintf(stderr,"set raw socket rx ring error \n");
    return -1;
  }

  ring_map_addr = mmap(NULL,req.tp_block_size * req.tp_block_nr,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_SHARED,sock_g,0);
  if(!ring_map_addr){
    fprintf(stderr,"map rx ring buf err\n");
    return -1;
  }
  
  *ring = malloc(req.tp_frame_nr * sizeof(struct iovec));
  if(!*ring){
    fprintf(stderr,"malloc iovec arry err\n");
    return -1;
  }
  for(i=0;i<req.tp_block_nr;i++){
    (*ring)[i].iov_base = ring_map_addr + (i*req.tp_block_size);
    (*ring)[i].iov_len = req.tp_block_size;
  }

  return 0;
}

void hexdump(unsigned char *addr,int len)
{
  int i;
  for(i=0;i<len;){
    if(i%16 == 0)
    printf("%04x: ",addr+i);
    printf("%02x ",addr[i]);
    i++;
    if(i%16 == 0)
      printf("\n");
  }
  if(i%16)
    printf("\n");
}

void walk_rx_ring(struct iovec *rx_ring,int ring_size,int *last)
{
  int i ;
  struct tpacket3_hdr *h;
  struct tpacket_block_desc *pbd;
  unsigned char *mac_hdr;

  for(i=*last;i<ring_size;){
    pbd = rx_ring[i].iov_base;

    if(!pbd->hdr.bh1.block_status&TP_STATUS_USER){
      *last = i;
      break;
    }
    
    int j;
    for(j =0,h = (void*)pbd+pbd->hdr.bh1.offset_to_first_pkt; j< pbd->hdr.bh1.num_pkts;j++){
      mac_hdr = (void*)h+h->tp_mac;
      //dump
      if(loglevel>9){
       hexdump(mac_hdr,h->tp_snaplen);
      }
      h = (void*)h + h->tp_next_offset;
    }
    pbd->hdr.bh1.block_status = TP_STATUS_KERNEL;
    __sync_synchronize();
    if(++i == ring_size)
      i = 0;
  }
}

int main_loop()
{
  fd_set rset;
  struct iovec *rx_ring;
  int ring_size = 0x100;
  int last = 0;

  if(raw_socket_zero_copy_setup(&rx_ring,&ring_size,0x800)){
      return -1;
  }

  for(;;){
    FD_ZERO(&rset);
    FD_SET(sock_g,&rset);
    if(1== select(sock_g+1,&rset,NULL,NULL,NULL)){
        walk_rx_ring(rx_ring,ring_size,&last);
    }else{
      break;
    }
  }
}

int
main(int argc, char *argv[])
{
    int opt;
    char *ifname = NULL;
    char *bpf_files = NULL;

    while ((opt = getopt(argc, argv, "i:b:l:h")) != -1) {
        switch (opt) {
        case 'i':
            ifname = strdup(optarg);
            break;
        case 'b':
            bpf_files = strdup(optarg);
            break;
        case 'l':
            loglevel = atoi(optarg);
            break;
        case 'h':
            return usage(argv[0],0);
        default: /* '?' */
            return usage(argv[0],-1);
        }
    }

    //create raw socket
    sock_g = socket(AF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
    if(sock_g < 0){
      perror("socket af packet create");
      return -1;
    }

    //socket  attr config 
    if(ifname && capture_packet_on_dev(ifname)){
      fprintf(stderr,"set capture on %s err\n",ifname);
      return -1;
    }

    if(bpf_files && apply_bpf_rule(bpf_files)){
      fprintf(stderr,"set bpf rule err\n",bpf_files);
      return -1;
    }

    main_loop();
    printf("Bye\n");  
    return 0;
}
