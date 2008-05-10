source config.sh

iptables --replace fluffycluster 2 --match state --state NEW -j NFQUEUE --queue-num $NFQUEUE_NUM


