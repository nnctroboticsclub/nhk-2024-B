use super::{
    transaction_result::TransactionError, urb_status::URBStatus, EPType, Transaction,
    TransactionDestination, TransactionResult,
};
use crate::common::{log, sleep_ms};
use alloc::format;

pub trait HC {
    fn new(dest: TransactionDestination, ep_type: EPType, max_packet_size: u32) -> Self;

    fn set_max_packet_size(&mut self, max_packet_size: u32);

    fn get_dest_mut(&mut self) -> &mut TransactionDestination;

    fn get_dest(&self) -> &TransactionDestination;

    fn submit_urb(&mut self, transaction: &mut Transaction) -> TransactionResult<()>;

    fn get_urb_status(&mut self) -> URBStatus;

    fn wait_done(&mut self) -> TransactionResult<()> {
        let mut tick: u32 = 0;
        while self.get_urb_status() == URBStatus::Idle {
            sleep_ms(1);

            if tick > 50 {
                return Err(TransactionError::Timeout);
            }

            tick += 1;
        }

        let status = self.get_urb_status();

        match status {
            URBStatus::Done => Ok(()),
            URBStatus::NotReady => Err(TransactionError::NotReady),
            URBStatus::Error => Err(TransactionError::Error),
            _ => {
                log(format!("Error: {:?}", status));
                Err(TransactionError::Error)
            }
        }
    }
}
