#!/bin/bash

# The following script will be invoked at shutdown by the 
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
echo "fc-finish:$IP"
if [ -z "$NETMASK" ]; then
	# Netmask not set
	NETMASK=24
fi

# Start by removing our IP alias
# This needs to be done first so that we don't inadvertently
# stuff up other nodes' connections
ip addr del dev $INTERFACE $IP/$NETMASK

# Remove our arptables rul
arptables -D OUTPUT --source-ip $IP -j DROP >/dev/null 2>&1

# Clean up our iptables mess..
iptables -D INPUT --destination $IP --in-interface $INTERFACE -j fluffycluster 2>&1 >/dev/null
iptables -F fluffycluster # Empty chain
iptables -X fluffycluster # delete chain

