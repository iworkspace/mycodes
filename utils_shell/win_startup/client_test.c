
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h> // _getcwd
#include <stdlib.h> // free, perror
#include <stdio.h>  // printf
#include <string.h> // strlen
#include <time.h>

#include <stdarg.h>

#include <Windows.h>

#ifndef WIN_64
#pragma comment(lib,"Ws2_32.lib")
#pragma comment(lib,"Advapi32.lib")
#else
#pragma comment(lib,"Ws2_64.lib")
#pragma comment(lib,"Advapi64.lib")
#endif

//#pragma comment(lib,"lualib.lib")

#pragma pack(1)
struct msg_s {
#define MAGIC 0x54545454
	int magic;
	unsigned short msglen;//buflen-sizeof(struct msg_s)
	int sn;//response +1.
	char data[0];
};

SERVICE_STATUS service_status;
SERVICE_STATUS_HANDLE service_handler_status;
SOCKET ssocket;
FILE *log_f = stdout;
char *bind_ipaddr="0.0.0.0";
char *bind_port="6677";
char *_pwd="C:\\win_startup";
char *dispatch_script="dispatcher.lua";


//for debug & foramt log out put
//int log_out(int level,char *fmt,...)
int log_out(char *fmt,...)
{
	va_list ap;
	int ret = -1;
	time_t time_now;
	char prefix[128];
	va_start(ap,fmt);

	time(&time_now);
	ret = strftime( prefix, sizeof(prefix), "[%Y/%m/%d %X] ",localtime(&time_now) );  
	sprintf(prefix+ret,"%s\n",fmt);

	if(log_f)
		ret = vfprintf(log_f,prefix,ap);
	va_end(ap);
	fflush(log_f);
	return ret;
}



void main_loop()
{
	int i;
	char buf[1600];
	struct msg_s *msg = buf;
	WSADATA ws;
	struct sockaddr_in bindaddr;


	//create socket & bind.
	if(WSAStartup(MAKEWORD(2,2),&ws)){
		log_out("init windows socket fail,info:%s",GetLastError());
		exit(-2);
	}

	ssocket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	
	msg->sn= htonl(100);
	msg->magic = MAGIC;
	sprintf(msg->data,"{'msgtype':'sshd'}");
	msg->msglen = htons(strlen(msg->data));

	memset(&bindaddr,0,sizeof(bindaddr));
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	bindaddr.sin_port = htons(atoi(bind_port));
	sendto(ssocket,buf,sizeof(struct msg_s)+strlen(msg->data),0,&bindaddr,sizeof(bindaddr));

	if(1<recvfrom(ssocket,buf,1600,0,&bindaddr,sizeof(bindaddr))){
			printf("11111111111%s\n",msg->data);
	}

}



void main(int argc,char *argv[])
{

	main_loop();
}
