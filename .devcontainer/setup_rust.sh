sudo apt update
sudo apt install -y libx11-6 clang
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh -s -- -y

. $HOME/.cargo/env

rustup override set nightly
rustup target add thumbv7em-none-eabi
rustup component add rust-src
cargo install cargo-binstall
yes | cargo binstall cargo-generate

rustup component add rustfmt clippy

wget https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/download/v13.2.1-1.1/xpack-arm-none-eabi-gcc-13.2.1-1.1-linux-x64.tar.gz -O /tmp/xpack.tgz
tar -xvf /tmp/xpack.tgz -C /tmp
sudo cp -r /tmp/xpack-arm-none-eabi-gcc-13.2.1-1.1 /opt
sudo ln -s /opt/xpack-arm-none-eabi-gcc-13.2.1-1.1/bin/arm-none-eabi-gcc /usr/bin/arm-none-eabi-gcc
sudo ln -s /opt/xpack-arm-none-eabi-gcc-13.2.1-1.1/bin/arm-none-eabi-gdb /usr/bin/arm-none-eabi-gdb
sudo ln -s /opt/xpack-arm-none-eabi-gcc-13.2.1-1.1/bin/arm-none-eabi-nm /usr/bin/arm-none-eabi-nm
sudo ln -s /opt/xpack-arm-none-eabi-gcc-13.2.1-1.1/bin/arm-none-eabi-objdump /usr/bin/arm-none-eabi-objdump
sudo ln -s /opt/xpack-arm-none-eabi-gcc-13.2.1-1.1/bin/arm-none-eabi-objcopy /usr/bin/arm-none-eabi-objcopy

wget https://github.com/xpack-dev-tools/qemu-arm-xpack/releases/download/v8.2.2-1/xpack-qemu-arm-8.2.2-1-linux-x64.tar.gz -O /tmp/xpack-qemu.tgz
tar -xvf /tmp/xpack-qemu.tgz -C /tmp
sudo cp -r /tmp/xpack-qemu-arm-8.2.2-1 /opt
sudo ln -s /opt/xpack-qemu-arm-8.2.2-1/bin/qemu-system-gnuarmeclipse /usr/bin/qemu-system-arm
