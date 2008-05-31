#!/bin/bash

# The following script will be invoked at startup by the 
# fluffycluster daemon. The following env. vars will
# be set:

# $INTERFACE - name of the interface we're using
# $IP - cluster's IP address
# $CLUSTERMAC - cluster's MAC address
# $NFQUEUE_NUM - netfilter queue number we're using
# $NETMASK - number of 1 bits in the netmask of
# our cluster IP network (set if possible)

# PATH may be set wrong. set it.

PATH=/sbin:/usr/sbin:/bin:/usr/bin

echo "fc-init:$IP"
if [ -z "$NETMASK" ]; then
	# Netmask not set
	NETMASK=24
fi
echo "NETMASK $NETMASK"

# Start by cleaning up any previous stuff
# We don't care much if these fail.
ip addr del dev $INTERFACE $IP/$NETMASK >/dev/null 2>&1
arptables -D OUTPUT --source-ip $IP -j DROP >/dev/null 2>&1

# Exit on error.
set -e 
# Now set up anew.
# Block the kernel's ARP responses on our IP.
arptables -A OUTPUT --source-ip $IP -j DROP
# Send our own arp replies...
# NB: this is done by arpd which is spawned anyway
# Add our own multicast addr...
# Not needed, we do this inside fluffycluster
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


set +e # Don't exit on error
# Add fluffycluster to the INPUT chain
# Remove if exists..
iptables -D INPUT --destination $IP --in-interface $INTERFACE -j fluffycluster >/dev/null 2>&1
set -e # Do exit on error
# Then put it back.
iptables -A INPUT --destination $IP --in-interface $INTERFACE -j fluffycluster

# Finally add our own IPV4 addr...
ip addr add dev $INTERFACE $IP/$NETMASK

