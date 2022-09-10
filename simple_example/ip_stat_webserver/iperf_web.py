#!/usr/bin/env python3
# -*- coding: utf-8 -*-
import os
import socket
import time
import threading
import yaml

from http.server import HTTPServer,SimpleHTTPRequestHandler
from urllib.parse import urlparse

stat_pps=0
stat_bps=0

sum_pkt_num=0
sum_pkt_bytes=0

def udp_iperf_stat_func(stat_perior=1000000000):
    global stat_pps
    global stat_bps
    last = time.time_ns()
    last_sum_num = sum_pkt_num
    last_sum_bytes = sum_pkt_bytes
    while True:
        now = time.time_ns()
        diff = now - last
        if diff > stat_perior :
            #print("last:%f now:%f" % (last,now))
            stat_pps = (sum_pkt_num-last_sum_num) / (diff/1000000000.0)
            stat_bps = (sum_pkt_bytes-last_sum_bytes) / (diff/1000000000.0)
            last = now
            last_sum_num = sum_pkt_num
            last_sum_bytes = sum_pkt_bytes
            #print("update stat %d %d  %d %d  @ %s " % (stat_pps,stat_bps,last_sum_num,last_sum_bytes,time.ctime(time.time())))
            continue
        else:
            time.sleep((stat_perior-diff)/1000000000.0)
            

def udp_iperf_rcv_func(ip='0.0.0.0',port=56334):
	global sum_pkt_num
	global sum_pkt_bytes
	socket_fd = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
	socket_fd.bind((ip,port))
	
	while True: 
		rcv,_n = socket_fd.recvfrom(2000)
		sum_pkt_num += 1
		sum_pkt_bytes += len(rcv)

def do_get_stat_handler(http_request): 
   http_request.set_headers(type='text/html',code=200)
   http_request.wfile.write(("%f,%f" % (stat_pps,stat_bps)).encode(encoding='utf-8'))
		   
class simple_httpserver(SimpleHTTPRequestHandler):
	def set_headers(self,code=200,type='text/html'):
		self.send_response(code)
		self.send_header('Content-type',type)
		self.end_headers()
		
	def do_GET(self):
		files=self.translate_path(self.path)
		if os.path.isfile(files) :
			f = open(files, 'rb')
			self.set_headers(type='text/html',code=200)
			self.wfile.write(f.read())
		elif os.path.isdir(files) and os.path.exists(files+"index.html"):
			f = open(files+"index.html", 'rb')
			self.set_headers(type='text/html',code=200)
			self.wfile.write(f.read())
		else:
			self.set_headers(type='text/html',code=404)

			self.wfile.write(bytes('''
				<html>
					<title> Not find </title>
					<body>
						<div>
							<h1>Sory,not find %s </h1>
						</div>
					</body>
				</html>
			''' % (files) ,encoding='utf-8'))
		
	def do_POST(self):
        # 简单enpoint 判断，判断是否为cgi后缀
		url_req = urlparse(self.path)
		word = url_req.path.split('/')
		if word[1] == 'get_stat':
			return do_get_stat_handler(self)
		elif word[1] == 'reset_stat':
			#do_reset_handler(self)
			pass
		else:
			pass

def webserver_func(ip='0.0.0.0',port=5454):
    server = HTTPServer((ip,port),simple_httpserver)
    server.serve_forever()

def read_yaml(fpath='config.yaml'):
    with open(fpath,"r") as f:
        return yaml.safe_load(f)

if __name__ == '__main__':
    configs = read_yaml("config.yaml")
    udp_rcv_thread = threading.Thread(target=udp_iperf_rcv_func,kwargs = {'port':configs["IPERF"]["bind_port"],'ip':configs["IPERF"]["bind_addr"]})
    stat_poll_thread = threading.Thread(target=udp_iperf_stat_func,kwargs={'stat_perior':configs["IPERF"]["poll_ns"]})
    webserver_thread = threading.Thread(target=webserver_func,kwargs={'port':configs["HTTP"]["bind_port"],'ip':configs["HTTP"]["bind_addr"]})
    stat_poll_thread.start()
    udp_rcv_thread.start()
    webserver_thread.start()
    udp_rcv_thread.join()
    stat_poll_thread.join()
    webserver_thread.join()

