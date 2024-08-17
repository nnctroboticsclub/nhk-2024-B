use alloc::{boxed::Box, format, string::String, vec};

use crate::{
    common::{log, sleep_ms},
    usb_core::{
        hc::{TransactionError, TransactionResult, HC},
        std_request::{Direction, Recipient, RequestByte, RequestKind, RequestType, StdRequest},
        Endpoint,
    },
};

use super::{PhysicalEP0, EP0};

pub struct USBEP0<H: HC> {
    ep: Endpoint<H>,
    dev: u8,
}

impl<H: HC> USBEP0<H> {
    pub fn new(hc: Box<H>) -> USBEP0<H> {
        USBEP0 {
            dev: hc.get_dest().dev,
            ep: Endpoint::new(hc),
        }
    }

    pub fn transaction_(
        &mut self,
        direction: Direction,
        req_type: RequestKind,
        recipient: Recipient,
        request: RequestByte,
        value: u16,
        index: u16,
        buf: &mut [u8],
    ) -> TransactionResult<()> {
        let is_out = direction == Direction::HostToDevice;

        let length = buf.len() as u16;

        self.ep.set_data_toggle(0);
        self.ep.send_setup(StdRequest {
            request_type: RequestType {
                direction,
                req_type,
                recipient,
            },
            request,
            value: value,
            index: index,
            length: length,
        })?;

        if is_out {
            if length != 0 {
                self.ep.send_packets(buf)?;
            }
            self.ep.set_data_toggle(1);
            self.ep.recv_packets(&mut [])?;
        } else {
            if length != 0 {
                self.ep.recv_packets(buf)?;
            }
            self.ep.set_data_toggle(1);
            self.ep.send_packets(&mut [])?;
        }
        Ok(())
    }

    pub fn transaction(
        &mut self,
        direction: Direction,
        req_type: RequestKind,
        recipient: Recipient,
        request: RequestByte,
        value: u16,
        index: u16,
        buf: &mut [u8],
    ) -> TransactionResult<()> {
        let mut retries = 0;
        let retry_limit = 50;

        loop {
            if retries >= retry_limit {
                return Err(TransactionError::Timeout);
            }
            retries += 1;

            match self.transaction_(direction, req_type, recipient, request, value, index, buf) {
                Ok(_) => break,
                Err(TransactionError::NotReady) => {
                    log(format!("Retry: {:?} NRDY", retries));
                    sleep_ms(50);
                    continue;
                }
                Err(TransactionError::Error) => {
                    log(format!("Retry: {:?} NERR", retries));
                    sleep_ms(50);
                    continue;
                }
                Err(e) => return Err(e),
            }
        }

        Ok(())
    }

    pub fn get_ep(&mut self) -> &mut Endpoint<H> {
        &mut self.ep
    }
}

impl<H: HC> PhysicalEP0 for USBEP0<H> {
    fn set_max_packet_size(&mut self, mps: u8) {
        self.ep.set_max_packet_size(mps);
    }

    fn set_address(&mut self, new_address: u8) -> TransactionResult<()> {
        self.transaction(
            Direction::HostToDevice,
            RequestKind::Standard,
            Recipient::Device,
            RequestByte::SetAddress,
            new_address.into(), // value = addr
            0x00_00,            // index = 0
            &mut [],            // buffer (dummy)
        )?;

        self.dev = new_address;
        self.ep.set_address(new_address)?;

        Ok(())
    }
}

impl<H: HC> EP0 for USBEP0<H> {
    fn get_descriptor(
        &mut self,
        descriptor_type: u8,
        index: u8,
        buf: &mut [u8],
    ) -> TransactionResult<()> {
        let value = (descriptor_type as u16) << 8 | (index as u16);
        let value = value.into();

        self.transaction(
            Direction::DeviceToHost,
            RequestKind::Standard,
            Recipient::Device,
            RequestByte::GetDescriptor,
            value,
            0x00_00,
            buf,
        )?;

        Ok(())
    }

    fn get_string(&mut self, index: u8) -> TransactionResult<String> {
        let length = {
            let buf = &mut [0; 2];
            self.get_descriptor(3, index, buf)?;
            buf[0] as usize
        };

        let mut buf = vec![0; length];

        self.get_descriptor(3, index, buf.as_mut_slice())?;

        let v = &buf[2..length];
        Ok(String::from_utf16le_lossy(v))
    }
}
