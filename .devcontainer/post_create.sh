#!/bin/bash

echo "export SSH_AUTH_SOCK=/workspaces/nhk-2024-b/.devcontainer/ssh-agent.sock" > ~/.bashrc

sudo ln -s /opt/gcc-arm-none-eabi-10.3-2021.10/bin/* /usr/bin/

sudo groupadd -g 986 uucp-2
sudo usermod -a -G uucp-2 vscode
curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.7/install.sh | bash

. ~/.nvm/nvm.sh
nvm install 20

WORKSPACE=`mount | grep /workspaces | awk '{print $3}'`

git -C $WORKSPACE submodule update --init --recursive

python3 -m pip install -r $WORKSPACE/common_libs/mbed-os/requirements.txt