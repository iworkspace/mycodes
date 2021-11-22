#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h> // _getcwd
#include <stdlib.h> // free, perror
#include <stdio.h>  // printf
#include <string.h> // strlen
#include <time.h>

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

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
FILE *log_f = NULL;
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

int class_msg_udp_send(lua_State *L) {

	struct sockaddr_in client_addr;
	char *ipaddr,*jsonstr;
	int port,sn,len,ret;
	SOCKET tmp_socket;
	char buf[1600];
	struct msg_s *msg = buf;

	//tmp_socket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	ipaddr = luaL_checkstring(L,1);
	port = luaL_checkinteger(L,2);
	sn = luaL_checkinteger(L,3);
	jsonstr = luaL_checkstring(L,4);
	
	//printf("%s@%d->%d,info:%s\n",ipaddr,port,sn,jsonstr);
	//response msg..
	msg->magic = MAGIC;
	msg->sn = htonl(sn);
	//len = snprintf(msg->data,1600-sizeof(struct msg_s),"%s",jsonstr);
	len = sprintf(msg->data,"%s",jsonstr);
	msg->msglen = htons(len);

	memset(&client_addr,0,sizeof(client_addr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = inet_addr(ipaddr);
	client_addr.sin_port = htons(port);

	//socket sendto call a invaild addr pars... ret success??????
	ret = sendto(ssocket /*tmp_socket*/,buf,sizeof(struct msg_s)+len,/*MSG_CONFIRM*/0,&client_addr,sizeof(client_addr));
	//closesocket(tmp_socket);
	//printf("ret:%d lenall:%d \n",ret,sizeof(struct msg_s)+len);
	if(ret != (sizeof(struct msg_s)+len) ){
		log_out("send msg err,info %s@%d->%d,info:%s\n",ipaddr,port,sn,jsonstr);
		lua_pushstring(L,"err");
		return 1;
	}

	return 0;
}

static const struct luaL_reg class_msg[] = {
	{"udp_send",class_msg_udp_send},
	{NULL,NULL}
};

int lua_handler(struct msg_s *msg,char *client_addr,int client_port,int sn)
{
	lua_State *L = lua_open();  /* create state */
	if (L == NULL) {
		log_out("lua err: cannot create state: not enough memory");
		return EXIT_FAILURE;
	}

	luaL_openlibs(L);  /* open libraries */

	/* socket wrap libs */
	luaL_register(L, "msg", class_msg);

	/* push global msg args */
	lua_pushinteger(L, sn);
	lua_setfield(L, -2, "sn");

	lua_pushstring(L, client_addr);
	lua_setfield(L, -2, "addr");

	lua_pushinteger(L, client_port);
	lua_setfield(L, -2, "port");

	lua_pushstring(L, msg->data);
	lua_setfield(L, -2, "json");

	if(luaL_dofile(L, dispatch_script) && !lua_isnil(L,-1)){
		char *errmsg = lua_tostring(L,-1);
		log_out("execute lua script err,info:%s",errmsg);
		lua_pop(L,1);
	}
	lua_close(L);
	return 0;
}

void win_startup_init(int argc,char *argv[])
{
	int i;
	char *buf;
	WSADATA ws;
	struct sockaddr_in bindaddr;

	for(i=1;i<argc-1;i++){
		if(!strcmp(argv[i],"-b")){
			bind_ipaddr=strdup(argv[++i]);
		}else if(!strcmp(argv[i],"-p")){
			bind_port=strdup(argv[++i]);
		}else if(!strcmp(argv[i],"-w")){
			_pwd=strdup(argv[++i]);
		}else if(!strcmp(argv[i],"-s")){
			dispatch_script=strdup(argv[++i]);
		}
	}
	
	//chage cwd
	if(_chdir(_pwd)){
		exit(-1);
	}
	//log_init.
	log_f=fopen("log.txt","w+");
	
	//dump configs
	buf=_getcwd(NULL,0);
	log_out("pwd is: %s",buf?buf:"unknow");
	if(buf)
		free(buf);

	log_out("bind info: %s:%s",bind_ipaddr,bind_port);
	log_out("callback script:%s",dispatch_script);

	//create socket & bind.
	if(WSAStartup(MAKEWORD(2,2),&ws)){
		log_out("init windows socket fail,info:%s",GetLastError());
		exit(-2);
	}

	ssocket = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
	
	memset(&bindaddr,0,sizeof(bindaddr));
	bindaddr.sin_family = AF_INET;
	bindaddr.sin_addr.s_addr = inet_addr(bind_ipaddr);
	bindaddr.sin_port = htons(atoi(bind_port));

	if(bind(ssocket,(struct sockaddr*)&bindaddr,sizeof(bindaddr))){
		log_out("bind socket fail,err info:%s",GetLastError());
		exit(-3);
	}
}

void main_loop()
{
	fd_set rset;
	SOCKET csocket;
	struct sockaddr_in client_addr;
	int socket_len = sizeof(client_addr);
	int num,len;
	char ipaddr[32];
	int port;
	char buf[1600];
	struct msg_s *msg = &buf;
	struct timeval timeout;
	for(;;){
		FD_ZERO(&rset);
		FD_SET(ssocket,&rset);
		num = select(ssocket+1,&rset,NULL,NULL,NULL);
		if(num>0){
			len=recvfrom(ssocket,buf,sizeof(buf),0,(struct sockaddr *)&client_addr,&socket_len);
			if(sizeof(struct msg_s) > len || (
				msg->magic != MAGIC || ntohs(msg->msglen)!= len - sizeof(struct msg_s) ) 
			){
				log_out("get wrong msg from %s:%d ",inet_ntoa(*(struct in_addr*)&client_addr.sin_addr.s_addr),
					ntohs(client_addr.sin_port));
				continue;
			}else{

				log_out("get msg from %s:%d, sn:%d",inet_ntoa(*(struct in_addr*)&client_addr.sin_addr.s_addr),ntohs(client_addr.sin_port),ntohl(msg->sn));
				lua_handler(msg,inet_ntoa(*(struct in_addr*)&client_addr.sin_addr.s_addr),ntohs(client_addr.sin_port),ntohl(msg->sn));
			}
		}
	}
}

void WINAPI simple_service_handler(DWORD request)
{
	switch(request){
	case SERVICE_CONTROL_STOP:
		service_status.dwCurrentState= SERVICE_STOPPED;
		break;
	case SERVICE_CONTROL_SHUTDOWN:
		service_status.dwCurrentState= SERVICE_STOPPED;
		break;
	default:
		break;
	}
	SetServiceStatus(service_handler_status,&service_status);
}

#ifndef WIN_64
void win_startup_service()
{
	service_status.dwServiceType = SERVICE_WIN32;
	service_status.dwCurrentState = SERVICE_START_PENDING;
	service_status.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN|SERVICE_ACCEPT_STOP;
	service_status.dwWin32ExitCode = 0 ;
	service_status.dwServiceSpecificExitCode = 0;
	service_status.dwCheckPoint = 0;
	service_status.dwWaitHint = 0;

	
	
	service_handler_status = RegisterServiceCtrlHandler("win_startup",simple_service_handler);

	if(!service_handler_status){
		log_out("create service_handler fail!");
		return;
	}

	service_status.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(service_handler_status,&service_status);

	log_out("start main loop....");
	main_loop();
}

#else
void win_startup_service()
{
	service_status.dwServiceType = SERVICE_WIN64;
	service_status.dwCurrentState = SERVICE_START_PENDING;
	service_status.dwControlsAccepted = SERVICE_ACCEPT_SHUTDOWN|SERVICE_ACCEPT_STOP;
	service_status.dwWin32ExitCode = 0 ;
	service_status.dwServiceSpecificExitCode = 0;
	service_status.dwCheckPoint = 0;
	service_status.dwWaitHint = 0;

	
	
	service_handler_status = RefisterServiceCtrlHandler("win_startup",simple_service_handler);

	if(!service_handler_status){
		log_out("create service_handler fail!");
		return;
	}

	service_status.dwCurrentState = SERVICE_RUNNING;
	SetServiceStatus(service_handler_status,&service_status);

	log_out("start main loop....");
	main_loop();
}

#endif

void main(int argc,char *argv[])
{
	SERVICE_TABLE_ENTRY entrys[2];
	entrys[0].lpServiceName = "win_startup";
	entrys[0].lpServiceProc=(LPSERVICE_MAIN_FUNCTION)win_startup_service;
	entrys[1].lpServiceName = NULL;
	entrys[1].lpServiceProc= NULL;
	win_startup_init(argc,argv);
	StartServiceCtrlDispatcher(entrys);
	//log_out("start main loop....");
	//main_loop();
}
