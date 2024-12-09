#!/bin/bash

make -C /opt/container libs_mbed

cargo binstall -y cbindgen