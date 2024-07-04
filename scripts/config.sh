set -e

cd $(dirname $0)/..
ROOT=$(pwd)

echo "Root: $ROOT"

[ -z "$FEP_DEVICE" ] & FEP_DEVICE=/dev/ttyUSB1
[ -z "$FEP_SOCKET" ] & FEP_SOCKET=$ROOT/.fep/fep.sock
[ -z "$FEP_PCAP" ] & FEP_PCAP=$ROOT/.fep/monitor.pcap

export FEP_DEVICE
export FEP_SOCKET

. .venv/bin/activate