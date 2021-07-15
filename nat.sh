#!/bin/bash
#wpa_supplicant -iwlan0 -c/etc/wpa_supplicant.conf &
#sleep 10
#dhclient wlan0 &
echo 1 > /proc/sys/net/ipv4/ip_forward
#export XTABLES_LIBDIR=/lib/xtables
iptables -t nat -A POSTROUTING -o wlan0 -j MASQUERADE
#echo 1 > /proc/sys/net/ipv4/conf/eth0/proxy_arp
killall -9 dhcpd
udhcpd
#dhcpd /etc/udhcpd_lan_nat.conf
