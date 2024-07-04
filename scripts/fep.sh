#!/bin/bash

. scripts/config.sh

cd syoch-robotics

[ -e $FEP_PCAP ] && rm $FEP_PCAP
mkfifo $FEP_PCAP

python3 -m utils.fep.fep_server &
PID_1=$!

sleep 0.5

python3 -m utils.fep.dump_pcap &
PID_2=$!

function trap_sigint() {
  printf "\nKilling process\n"
  kill -9 $PID_1
  kill -9 $PID_2
  [ -e output.pcap ] && rm output.pcap
}

trap trap_sigint EXIT

wait