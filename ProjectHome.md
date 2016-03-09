# What it does #

This software will create a "virtual IP address" shared between two or more machines on the same shared segment (i.e. LAN). TCP-based services running will be automatically load-balanced between the available machines.

No dedicated "load balancer" is required.

# Load-balancing #

We do automatic load balancing between all of the available nodes in the cluster. Nodes are automatically discovered and "heartbeat" packets used so that the other nodes know they're still there.

# Fail-over #

Nodes which have failed are automatically taken out of the cluster, i.e. connections will not be sent there.

# Other features #

  * Minimal configuration - just give it an interface and the shared IP address
  * Same configuration on all nodes - no "node ID" or "Cluster ID" to configure per node.
  * Node auto-discovery.
  * No planning required for the number of nodes which will be added in the future. For example, unlike CLUSTERIP, it is not necessary to know how many nodes there are in the cluster.
  * Nodes can be added/removed without downtime.

# Requirements #

  * Linux 2.6 kernel
  * IPtables, connection tracking, arp tables.
  * Currently a (very small) kernel module is required to change the behaviour of Linux. This is < 50 lines of code.
  * Each node needs to have an IP address on the shared network which the cluster IP address is on. This need not be a public IP address, but routing should really work upstream so that we can send packets out.

# Status #

A functional prototype. Not suitable for production use.

# Trying it out #

Currently there are no builds. The source code is in svn here. If you intend to try it out, please do so under virtual machines where no harm can be done. Ensure that you use it only on a virtual network, or a network not vital for production use. I can't guarantee that the network behaviour of this software won't break your network, even if you use virtual machines :)

Comments to markxr@gmail.com please.