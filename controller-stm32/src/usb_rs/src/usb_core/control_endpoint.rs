use core::fmt::{self, Debug};

use alloc::{format, vec, vec::Vec};

use crate::{
    common::{log, sleep_ms},
    usb_core::std_request::{Direction, StdRequest},
};

use super::{
    hc::{TransactionError, TransactionResult, HC},
    std_request::{Recipient, RequestByte, RequestKind, RequestType},
    Endpoint,
};

pub struct ControlTransfer<'a> {
    pub request: StdRequest,
    pub data: &'a mut [u8],
}

impl<'a> Debug for ControlTransfer<'a> {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        write!(
            f,
            "{}{{{:3?};",
            match self.request.request_type.direction {
                Direction::HostToDevice => "\x1b[32m ControlRead\x1b[m",
                Direction::DeviceToHost => "\x1b[31mControlWrite\x1b[m",
            },
            self.data.len(),
        )?;

        let req: [u8; 8] = self.request.clone().into();
        write!(
            f,
            " \x1b[35m{:02X}{:02X}{:02X}{:02X} {:02X}{:02X}{:02X}{:02X} \x1b[1;37m| \x1b[m",
            req[0], req[1], req[2], req[3], req[4], req[5], req[6], req[7]
        )?;

        for i in 0..self.data.len() {
            if i % 4 == 0 {
                write!(f, " \x1b[{}m", if i & 4 == 0 { 34 } else { 36 })?;
            }
            write!(f, "{:02X}", self.data[i as usize])?;

            if i % 4 == 3 {
                write!(f, "\x1b[0m")?;
            }
        }

        write!(f, "\x1b[m]}}")
    }
}

pub struct ControlRequest {
    pub req_type: RequestKind,
    pub recipient: Recipient,

    pub request: RequestByte,
    pub value: u16,
    pub index: u16,
}

pub struct ControlEndpoint<H: HC> {
    pub endpoint: Endpoint<H>,
}

impl<H: HC> ControlEndpoint<H> {
    pub fn new(endpoint: Endpoint<H>) -> ControlEndpoint<H> {
        ControlEndpoint { endpoint }
    }

    fn control_transfer_(
        &mut self,
        buffer: &mut [u8],
        direction: Direction,
        request: &ControlRequest,
    ) -> TransactionResult<()> {
        let direction = direction;

        let req = StdRequest {
            request_type: RequestType {
                direction,
                req_type: request.req_type,
                recipient: request.recipient,
            },
            request: request.request,
            value: request.value,
            index: request.index,
            length: buffer.len() as u16,
        };

        let transfer = ControlTransfer {
            request: req,
            data: buffer,
        };

        self.endpoint.send_setup(transfer.request)?;

        match direction {
            Direction::HostToDevice => {
                if transfer.data.len() != 0 {
                    self.endpoint.send_packets(transfer.data)?;
                }
                self.endpoint.recv_packets(&mut [])?;
            }
            Direction::DeviceToHost => {
                if transfer.data.len() != 0 {
                    self.endpoint.recv_packets(transfer.data)?;
                }
                self.endpoint.send_packets(&mut [])?;
            }
        }

        Ok(())
    }

    fn control_transfer(
        &mut self,
        buffer: &mut [u8],
        direction: Direction,
        request: ControlRequest,
    ) -> TransactionResult<()> {
        let mut retries = 0;
        let retry_limit = 50;

        loop {
            if retries >= retry_limit {
                return Err(TransactionError::Timeout);
            }
            retries += 1;

            match self.control_transfer_(buffer, direction, &request) {
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

    pub fn control_read(
        &mut self,
        length: u16,
        request: ControlRequest,
    ) -> TransactionResult<Vec<u8>> {
        let mut buffer = vec![0; length as usize];

        self.control_transfer(buffer.as_mut_slice(), Direction::DeviceToHost, request)?;

        Ok(buffer)
    }

    pub fn control_write(&mut self, request: ControlRequest, data: &[u8]) -> TransactionResult<()> {
        self.control_transfer(
            data.to_vec().as_mut_slice(),
            Direction::HostToDevice,
            request,
        )
    }
}
