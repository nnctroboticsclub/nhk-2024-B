#!/bin/bash

bash $(dirpath $0)/setup_rust.sh

sudo apt update
sudo apt install -y socat stlink-tools librsvg2-bin

. $(dirpath $0)/setup_rust.sh

ls -s /opt/gcc-arm-none-eabi-10.3-2021.10/bin | xargs -I {} sudo rm /usr/bin/{}
wget https://developer.arm.com/-/media/Files/downloads/gnu/13.3.rel1/binrel/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi.tar.xz -O /tmp/gcc-arm-none-eabi-10.3-2021.10.tar.xz
sudo tar -xf /tmp/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi.tar.xz /opt
sudo ln  /opt/arm-gnu-toolchain-13.3.rel1-x86_64-arm-none-eabi/bin/arm-none-eabi-* /usr/bin/

sudo groupadd -g 986 uucp-2
sudo usermod -a -G uucp-2 vscode
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.7/install.sh | bash

. ~/.nvm/nvm.sh
nvm install 20

WORKSPACE=$(mount | grep /workspaces | awk '{print $3}')

git -C $WORKSPACE submodule update --init --recursive

mkdir -p /tmp/setup || true

git clone git@github.com:nnctroboticsclub/libs-cmake.git /tmp/libs-cmake
git clone git@github.com:nnctroboticsclub/static-mbed-os.git /tmp/static-mbed-os

make -C /tmp/libs-cmake install
make -C /tmp/static-mbed-os install TARGET=NUCLEO_F446RE