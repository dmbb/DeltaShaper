#!/bin/bash 
getMyIP() {
    local _ip _myip _line _nl=$'\n'
    while IFS=$': \t' read -a _line ;do
        [ -z "${_line%inet}" ] &&
           _ip=${_line[${#_line[1]}>4?1:2]} &&
           [ "${_ip#127.0.0.1}" ] && _myip=$_ip
      done< <(LANG=C /sbin/ifconfig)
    printf ${1+-v} $1 "%s${_nl:0:$[${#1}>0?0:1]}" $_myip
}
getMyIP varHostIP
sudo ip netns add TEST
sudo ip link add veth0 type veth peer name veth1
sudo ip link set dev veth1 netns TEST
sudo ip link set dev veth0 up
sudo ip netns exec TEST ip link set dev veth1 up
sudo ip netns exec TEST ip addr add 10.10.10.10/32 dev veth1
sudo ip route add 10.10.10.10/32 dev veth0
sudo ip netns exec TEST ip route add $varHostIP/32 dev veth1
sudo ip netns exec TEST route add default gw $varHostIP
sudo ip addr add 10.10.10.11/32 dev veth0
