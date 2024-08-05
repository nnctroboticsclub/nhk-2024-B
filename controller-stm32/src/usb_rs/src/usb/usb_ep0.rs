use alloc::{boxed::Box, string::String, vec};

use crate::usb_core::{
    hc::{TransactionResult, HC},
    std_request::{Direction, Recipient, RequestByte, RequestKind, RequestType, StdRequest},
    ControlEP,
};

use super::{PhysicalEP0, EP0};

pub struct USBEP0<H: HC> {
    ep: ControlEP<H>,
    dev: u8,
}

impl<H: HC> USBEP0<H> {
    pub fn new(hc: Box<H>) -> USBEP0<H> {
        USBEP0 {
            dev: hc.get_dest().dev,
            ep: ControlEP::new(hc),
        }
    }

    fn transaction(
        &mut self,
        direction: Direction,
        req_type: RequestKind,
        recipient: Recipient,
        request: RequestByte,
        value: u16,
        index: u16,
        buf: &mut [u8],
        length: u16,
    ) -> TransactionResult<()> {
        let is_out = direction == Direction::HostToDevice;

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
                self.ep.send_packets(buf, length.into())?;
            }
            self.ep.set_data_toggle(1);
            self.ep.recv_packets(buf, 0)?;
        } else {
            if length != 0 {
                self.ep.recv_packets(buf, length.into())?;
            }
            self.ep.set_data_toggle(1);
            self.ep.send_packets(buf, 0)?;
        }
        Ok(())
    }
}

impl<H: HC> PhysicalEP0 for USBEP0<H> {
    fn set_max_packet_size(&mut self, mps: u8) {
        self.ep.set_max_packet_size(mps);
    }

    fn set_address(&mut self, new_address: u8) -> TransactionResult<()> {
        let mut buf = [0u8; 0];
        self.transaction(
            Direction::HostToDevice,
            RequestKind::Standard,
            Recipient::Device,
            RequestByte::SetAddress,
            new_address.into(), // value = addr
            0x00_00,            // index = 0
            &mut buf,           // buffer (dummy)
            0,                  // length = 0
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
        length: u16,
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
            length,
        )?;

        Ok(())
    }

    fn get_string(&mut self, index: u8) -> TransactionResult<String> {
        let length = {
            let buf = &mut [0; 2];
            self.get_descriptor(3, index, buf, 2)?;
            buf[0] as usize
        };

        let mut buf = vec![0; length];

        self.get_descriptor(3, index, buf.as_mut_slice(), length.try_into().unwrap())?;

        let v = &buf[0..length];
        Ok(String::from_utf16le_lossy(v))
    }
}
