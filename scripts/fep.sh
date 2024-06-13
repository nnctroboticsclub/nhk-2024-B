#!/bin/bash

set -e

cd syoch-robotics

[ -e output.pcap ] && rm output.pcap
mkfifo output.pcap

function task1() {
  python3 -m utils.fep.fep_server
}

function task2() {
  python3 -m utils.fep.dump_pcap 0.0.0.0 output.pcap
}

task1 &
PID_1=$!

sleep 0.1

task2 &
PID_2=$!

function trap_sigint() {
  printf "\nKilling process\n"
  kill -9 $PID_1
  kill -9 $PID_2
  [ -e output.pcap ] && rm output.pcap
}

trap trap_sigint INT

wait