#!bin/bash

#Para executar e preciso dar permissao ao ficheiro "chmod +x tux61.sh"
updateimage
/etc/init.d/networking restart

ifconfig eth0 up
ifconfig eth0 172.16.60.1/24
route add -net 172.16.61.0/24 gw 172.16.60.254
route add default gw 172.16.60.254
