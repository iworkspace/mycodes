#!/bin/bash
#set +e
is_once=$1
dns_name=`git config ddns.domain`
dns_token=`git config ddns.token`

if [ -z "$dns_name" ] || [ -z "$dns_token" ] ; then
	echo "miss ddns cfg."
	exit 1
fi

while : ; do
	#interface_ipv6=`ip a | awk  '/inet6/{print $2}'| awk -F'/' '/^2[0-9a-f:]/{print $1}'`
	#linux use temopray ipv6 addr first.
	interface_ipv6=`ip a | grep 'temporary' | awk  '/inet6/{print $2}' |awk -F'/' '/^2[0-9a-f:]/{print $1}'`
	if [  -z "$interface_ipv6" ] ; then
		echo "no ipv6 addr ..."
		exit 1
	fi
	
	#ddns_ipv6=`nslookup workspace.noip.cn ? iworkspace.noip.cn | grep -A 1 "Name:" |grep "Address:"| awk '{print $2}'`
	ddns_ipv6=`nslookup $dns_name | grep -A 1 "Name:" |grep "Address:"| awk '{print $2}'`
	if [ "$interface_ipv6" != "$ddns_ipv6"  ] ; then
		wget "http://www.meibu.com/ipv6zdz.asp?ipv6=$interface_ipv6&name=$dns_name&pwd=$dns_token" -o ddns_v6_push.log -O ddns_v6.result
		sed -i 's/\r//' ddns_v6.result
		result=`cat ddns_v6.result`
		if [ $result == "ok" ] ; then
			echo "set ddns.. ok "
			if [ ! -z "${is_once}" ] ; then
				echo "no change..."
				exit 0
			fi
		else
			echo "set ddns .. error "
			sleep 30
		fi
	else
		if [ ! -z "${is_once}" ] ; then
			echo "no change..."
			exit 0
		fi
		#echo "no change."
		sleep 30
	fi
done
