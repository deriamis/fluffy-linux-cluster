# Advantages of Fluffy Cluster #

Advantages over (for example) Linux Virtual Server:

  * Fewer machines required (no dedicated load balancer; using LVS, load balancers cannot **easily** also be real servers)
  * Much simpler configuration
  * Completely automatic failover
  * A single node failing affects only connections to that node vs. if a load balancer fails, all connections will be lost even if a hot spare takes over immediately (A method exists in LVS which can apparently get around this?)

# Disadvantages #

Over LVS

  * May perform less well
  * Routers which respect RFC1812's recommendation may be incompatible
  * Highly experimental vs LVS very well tested