#!bin/bash

#Para executar e preciso dar permissao ao ficheiro "chmod +x tux62.sh"

updateimage
/etc/init.d/networking restart

ifconfig eth0 up
ifconfig eth0 172.16.61.1/24
route add -net 172.16.60.0/24 gw 172.16.61.253
route add default gw 172.16.61.254

#Experiencia 4 ponto 4
#echo 0 > /proc/sys/net/ipv4/conf/eth0/accept_redirects
#echo 0 > /proc/sys/net/ipv4/conf/all/accept_redirects
