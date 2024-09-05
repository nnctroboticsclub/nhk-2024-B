#![feature(sync_unsafe_cell)]

use std::time::Duration;

use im920_rs::{Error, Packet, IM920};
use srobo_base::{
    communication::{AsyncSerial, SerialDevice},
    time::HostTime,
};

fn main() {
    let (mut dev_rx, mut dev_tx) = SerialDevice::new(
        std::env::var("DEV").expect("serial port device $DEV is not set"),
        19200,
    )
    .open()
    .unwrap();

    let mut time = HostTime::new();

    let mut app = IM920::new(&mut dev_tx, &mut dev_rx, &mut time);
    let nn = app.get_node_number(Duration::from_secs(5)).unwrap();
    println!("Node Number: {}", nn);

    println!("Version: {:?}", app.get_version(Duration::from_secs(5)));

    app.on_data(Box::new(|msg| {
        println!(
            "{:?} <-- {:?} ({:?})",
            msg.packet.data, msg.packet.node_id, msg.rssi
        );
    }));

    loop {
        match app.transmit_delegate(
            Packet {
                node_id: 3 - nn,
                data: vec![0, 1, 2, 3],
            },
            Duration::from_secs(5),
        ) {
            Ok(()) => (),
            Err(Error::OperationFailed) => (),
            Err(Error::Timeout) => panic!("Timeout"),
            Err(e) => panic!("Error: {:?}", e),
        }

        std::thread::sleep(Duration::from_secs(1));
    }
}
