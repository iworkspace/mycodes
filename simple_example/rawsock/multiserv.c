#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdio.h>
#ifdef __FreeBSD__
#include <net/if.h>
#include <net/if_dl.h>
#else
#include <linux/if.h>
#endif
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>


int sock_g = -1;


int usage(char *prog,int ret){
  printf("%s < -i interface > < -g group_address:port >  < -s source_address:port > [ -h ] \n",prog);
  return ret;
}

void main_loop()
{
  
}

int parse_ip_port(char *address,uint32_t *ipaddr,int *port)
{
  char ip_buf[128];
  if(2!=sscanf(address,"%[^:]:%d",&ip_buf,port)){
    return -1;
  }
  inet_pton(AF_INET,ip_buf,ipaddr);
  *port= htons(*port);
  return 0;
}

int
main(int argc, char *argv[])
{
    int opt;
    char *interface = NULL;
    uint32_t group_ip = NULL;
    uint32_t source_ip = NULL;
    int group_port = -1;
    int source_port = -1;

    while ((opt = getopt(argc, argv, "i:g:s:h")) != -1) {
        switch (opt) {
        case 'i':
            interface = strdup(optarg);
            break;
        case 'g':
            if(parse_ip_port(optarg,&group_ip,&group_port)){
                return usage(argv[0],-1);
            }
            break;
         case 's':
            if(parse_ip_port(optarg,&source_ip,&source_port)){
                return usage(argv[0],-1);
            }
            break;
        case 'h':
            return usage(argv[0],0);
        default: /* '?' */
            return usage(argv[0],-1);
        }
    }
    

    if(!interface || group_ip == -1 || group_port == -1 || source_port == -1 || source_port == -1 ){
        return usage(argv[0],-1);
    }

    //create multicast socket
    sock_g = socket(AF_INET,SOCK_DGRAM,0);
    if(sock_g < 0){
      printf("socket create faile\n");
      return -1;
    }

    //bind attr
    struct sockaddr_in sock_attr;
    socklen_t sock_len = sizeof(sock_attr);
    sock_attr.sin_family = AF_INET;
    sock_attr.sin_port = source_port;
    *(unsigned int*)&sock_attr.sin_addr = source_ip;

    bind(sock_g,&sock_attr,sock_len);

    //socket pair set up
    sock_attr.sin_port = group_port ;
    *(unsigned int*)&sock_attr.sin_addr = group_ip;
    
    struct ip_mreqn muti;
    *(unsigned int*)&muti.imr_multiaddr.s_addr;
    inet_pton(AF_INET,"0.0.0.0",&muti.imr_address.s_addr);
    muti.imr_ifindex = if_nametoindex(interface);
    setsockopt(sock_g,IPPROTO_IP,IP_MULTICAST_IF,&muti,sizeof(muti));

    //recv & echo reply.
    char buf [1024];
    sendto(sock_g,buf,1024,0,&sock_attr,sizeof(sock_attr));
   // main_loop();
}
