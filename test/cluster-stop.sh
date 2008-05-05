IP=192.168.70.42
NETMASK=24
INTERFACE=eth0
MAC=01:00:5e:00:00:28

# Start by cleaning up any previous stuff
ip addr del dev $INTERFACE $IP/$NETMASK
killall arpd
ip maddr del dev $INTERFACE $MAC
# Clean up our iptables mess..
iptables -D INPUT --destination $IP --in-interface $INTERFACE -j fluffycluster 2>&1 >/dev/null
iptables -F fluffycluster
iptables -X fluffycluster

# Clean up arptables
arptables -F OUTPUT

# All done?
