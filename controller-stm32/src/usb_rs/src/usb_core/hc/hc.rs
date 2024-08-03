use crate::common::sleep_ms;

use super::{
    transaction_result::TransactionError, urb_status::URBStatus, EPType, Transaction,
    TransactionDestination, TransactionResult,
};

pub trait HC {
    fn new(dest: TransactionDestination, ep_type: EPType, max_packet_size: i32) -> Self;

    fn submit_urb(&mut self, transaction: &mut Transaction) -> TransactionResult;

    fn get_urb_status(&mut self) -> URBStatus;

    fn wait_done(&mut self) -> TransactionResult {
        let mut tick: u32 = 0;
        while self.get_urb_status() != URBStatus::Done {
            sleep_ms(10);

            if tick > 100 {
                return Err(TransactionError::Timeout);
            }

            tick += 1;
        }

        Ok(())
    }
}
