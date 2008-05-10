source config.sh

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
