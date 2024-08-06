use super::HC;

pub struct FakeHC {
    done_timer_fired: std::time::Instant,
    dest: super::TransactionDestination,

    pub request_fired: u32,
}

impl HC for FakeHC {
    fn get_urb_status(&mut self) -> super::urb_status::URBStatus {
        if self.done_timer_fired.elapsed().as_secs() > 1 {
            super::urb_status::URBStatus::Done
        } else {
            super::urb_status::URBStatus::NYet
        }
    }

    fn new(
        dest: super::TransactionDestination,
        _ep_type: super::EPType,
        _max_packet_size: i32,
    ) -> Self {
        FakeHC {
            done_timer_fired: std::time::Instant::now(),
            dest: dest,
            request_fired: 0,
        }
    }

    fn get_dest_mut(&mut self) -> &mut super::TransactionDestination {
        &mut self.dest
    }

    fn get_dest(&self) -> &super::TransactionDestination {
        &self.dest
    }

    fn submit_urb(&mut self, transaction: &mut super::Transaction) -> super::TransactionResult<()> {
        self.done_timer_fired = std::time::Instant::now();

        println!("FakeHC::submit_urb: {:?}", transaction);
        self.request_fired += 1;

        if !transaction.token.is_outgoing_token() {
            for i in 0..transaction.length as usize {
                transaction.buffer[i] = i as u8;
            }
        }

        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use crate::usb_core::hc::{
        EPType, Transaction, TransactionDestination, TransactionError, TransactionToken,
    };

    use super::*;

    #[test]
    fn test_fake_hc() {
        let dest = TransactionDestination { dev: 0, ep: 0 };
        let mut hc = FakeHC::new(dest, EPType::Control, 8);

        let mut transaction = Transaction {
            token: TransactionToken::Setup,
            toggle: 0,
            buffer: &mut [0; 8],
            length: 8,
        };

        hc.submit_urb(&mut transaction).unwrap();

        assert_eq!(hc.request_fired, 1);
    }
}
