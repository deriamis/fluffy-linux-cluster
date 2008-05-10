source config.sh

# Start by cleaning up any previous stuff
ip addr del dev $INTERFACE $IP/$NETMASK 2>&1 >/dev/null
killall arpd 2>&1 >/dev/null
ip maddr del dev $INTERFACE $MAC 2>&1 >/dev/null
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
# Second rule in fluffycluster should  match NEW. We will flip this from
# DROP to ACCEPT When we're ready to accept.
iptables -A fluffycluster --match state --state NEW -j DROP
iptables -A fluffycluster --match state --state INVALID -j DROP
iptables -A fluffycluster --match state --state ESTABLISHED,RELATED -j ACCEPT
# Nothing should really get this far, but if it does, ignore it.
iptables -A fluffycluster -j DROP


# Add fluffycluster to the INPUT chain
# Remove if exists..
iptables -D INPUT --destination $IP --in-interface $INTERFACE -j fluffycluster 2>&1 >/dev/null
# Then put it back.
iptables -A INPUT --destination $IP --in-interface $INTERFACE -j fluffycluster

# Finally add our own IPV4 addr...
ip addr add dev $INTERFACE $IP/$NETMASK

