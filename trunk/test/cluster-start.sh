source config.sh

# Start by cleaning up any previous stuff
ip addr del dev $INTERFACE $IP/$NETMASK >/dev/null 2>&1
killall arpd >/dev/null 2>&1
ip maddr del dev $INTERFACE $MAC >/dev/null 2>&1
arptables -F OUTPUT

# Now set up anew.
# Block the kernel's ARP responses on our IP.
arptables -A OUTPUT --source-ip $IP -j DROP
# Send our own arp replies...
./arpd $INTERFACE $IP $MAC &
# Add our own multicast addr...
ip maddr add dev $INTERFACE $MAC
# Set up iptables
iptables -N fluffycluster
iptables -F fluffycluster
# First rule drops new things which are TCP but not SYNs.
iptables -A fluffycluster --protocol tcp --match state --state NEW ! --syn -j DROP
# Second rule in fluffycluster should  match NEW. This sends it to an NFQUEUE
# which is picked up by the daemon.
iptables -A fluffycluster --match state --state NEW -j NFQUEUE --queue-num $NFQUEUE_NUM
# Drop invalid stuff (maybe handled by another node)
iptables -A fluffycluster --match state --state INVALID -j DROP
# Accept established / related - as it must be for us if we get this far.
iptables -A fluffycluster --match state --state ESTABLISHED,RELATED -j ACCEPT
# Nothing should really get this far, but if it does, ignore it.
iptables -A fluffycluster -j DROP


# Add fluffycluster to the INPUT chain
# Remove if exists..
iptables -D INPUT --destination $IP --in-interface $INTERFACE -j fluffycluster >/dev/null 2>&1
# Then put it back.
iptables -A INPUT --destination $IP --in-interface $INTERFACE -j fluffycluster

# Finally add our own IPV4 addr...
ip addr add dev $INTERFACE $IP/$NETMASK

