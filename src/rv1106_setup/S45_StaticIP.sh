#!/bin/sh


MAX_TRIES=10
TRIES=0


check_ip_address() {

  if ifconfig eth0 | grep -q "inet "; then
    return 0
  else
    return 1
  fi
}

static_ip()
{            
  
while [ $TRIES -lt $MAX_TRIES ]; do
  if check_ip_address; then
    echo "DHCP succeed!"
    ifconfig eth0 192.168.1.200 netmask 255.255.255.0
    route add default gw 192.168.1.1
    echo "nameserver 114.114.114.114" > /etc/resolv.conf
    ifconfig eth0 up 
    break
  else
    echo "wait DHCP IP..."
    TRIES=$((TRIES + 1))
    sleep 5 
  fi
done


if [ $TRIES -eq $MAX_TRIES ]; then
  echo "error"

fi
}
case $1 in
        start)
                echo "start"
                static_ip
                ;;
        stop)
                echo "stop"
                ;;
        *)
                exit 1
                ;;
esac
