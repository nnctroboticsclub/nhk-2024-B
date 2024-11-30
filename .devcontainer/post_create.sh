#!/bin/bash

make -C /opt/container setup_rust

. ~/.cargo/env
cargo-binstall cbindgen

make -C /opt/container libc_mbed