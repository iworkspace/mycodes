from socket import *
import struct
import json


''' configs  '''
win_addr='127.0.0.1'
win_port=6677
my_ip="127.0.0.1"

services = {
    'sshd': {
        'ip': my_ip,
        'port': 22
    },
    'sambd': {
        'ip':my_ip,
        'path': '/workspace'
    }
}

msg_sn=0;

sock_fd = socket(AF_INET,SOCK_DGRAM);

''' todo get service status '''
''' todo msg ack. '''
for key,s in services.items() :
    if s.get('status') == None:     
        cfg = { 'services':key,'configs':s}
        cfg = bytes(json.dumps(cfg),'utf-8')
        msg_strdef='!iHi'+str(len(cfg)+1)+'s'
        data = struct.pack(msg_strdef,0x54545454,len(cfg)+1,msg_sn,cfg)
        sock_fd.sendto(data,(win_addr,win_port))
        s['status'] = 'send'

    
