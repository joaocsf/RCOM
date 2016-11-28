#!bin/bash

#Para executar e preciso dar permissao ao ficheiro "chmod +x tux64.sh"

updateimage
/etc/init.d/networking restart
echo 0 > /proc/sys/net/ipv4/icmp_echo_ignore_broadcasts
echo 1 > /proc/sys/net/ipv4/ip_forward
ifconfig eth0 up
ifconfig eth0 172.16.60.254/24

ifconfig eth1 up
ifconfig eth1 172.16.61.253/24
route add default gw 172.16.61.254
