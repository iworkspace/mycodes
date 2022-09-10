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

#class jsonObj(dict):
#    __setattr__=dict.__setattr__
#    __getattr__=dict.__getattr__
#
#def TojsonObj(dobj):
#    if not isinstance(dobj,dict):
#        return dobj
#    d=dict()
#    for k,v in dobj.items():
#        d[k]=TojsonObj(v)
#    return d

msg_sn=0;
sock_fd = socket(AF_INET,SOCK_DGRAM);

''' todo get service status '''
''' todo msg ack. '''
for key,s in services.items() :
    if s.get('status') == None:     
        cfg = { 'services':key,'configs':s}
        cfg = json.dumps(cfg).encode()
        msg_strdef='!iHi'+str(len(cfg))+'sx'
        data = struct.pack(msg_strdef,0x54545454,len(cfg)+1,msg_sn,cfg)
        sock_fd.sendto(data,(win_addr,win_port))
        s['status'] = 'send'

   


