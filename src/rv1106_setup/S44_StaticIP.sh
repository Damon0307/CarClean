#!/bin/sh

IP_ADDRESS="192.168.2.79"
NETMASK="255.255.255.0"
GATEWAY="192.168.2.1"
DNS_SERVER="114.114.114.114"

setup_static_ip() {
  echo "Configuring static IP address..."
  ifconfig eth0 $IP_ADDRESS netmask $NETMASK
  route add default gw $GATEWAY
  echo "nameserver $DNS_SERVER" > /etc/resolv.conf
  ifconfig eth0 up
  echo "Static IP configuration complete."
}

case $1 in
        start)
                echo "Starting static IP configuration..."
                setup_static_ip
                ;;
        stop)
                echo "Stopping static IP configuration is not applicable."
                ;;
        *)
                echo "Usage: $0 {start}"
                exit 1
                ;;
esac