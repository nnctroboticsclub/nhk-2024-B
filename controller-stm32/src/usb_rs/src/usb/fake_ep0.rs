use core::panic;

use alloc::{collections::btree_map::BTreeMap, string::String, vec::Vec};

use crate::usb_core::hc::TransactionResult;

use super::{PhysicalEP0, EP0};

#[derive(Ord, PartialOrd, Eq, PartialEq, Debug)]
pub enum DescriptorType {
    Device,
    Configuration,
    String,
    Interface,
    Endpoint,
}

impl From<u8> for DescriptorType {
    fn from(value: u8) -> Self {
        match value {
            1 => DescriptorType::Device,
            2 => DescriptorType::Configuration,
            3 => DescriptorType::String,
            4 => DescriptorType::Interface,
            5 => DescriptorType::Endpoint,
            _ => panic!("Unknown descriptor type: {}", value),
        }
    }
}

#[derive(PartialEq, Ord, PartialOrd, Eq, Debug)]
pub struct DescriptorAddress {
    pub kind: DescriptorType,
    pub index: u8,
}

#[derive(Default)]
pub struct FakeEP0 {
    descriptors: BTreeMap<DescriptorAddress, Vec<u8>>,
    strings: BTreeMap<u8, String>,
}

impl FakeEP0 {
    pub fn add_descriptor(&mut self, kind: DescriptorType, index: u8, data: Vec<u8>) {
        let address = DescriptorAddress { kind, index };
        self.descriptors.insert(address, data);
    }

    pub fn add_string(&mut self, index: u8, data: String) {
        self.strings.insert(index, data);
    }
}

impl EP0 for FakeEP0 {
    fn get_descriptor(
        &mut self,
        descriptor_type: u8,
        index: u8,
        buf: &mut [u8],
    ) -> TransactionResult<()> {
        let address = DescriptorAddress {
            kind: descriptor_type.into(),
            index,
        };

        if !self.descriptors.contains_key(&address) {
            panic!("Descriptor not found: {:?}", address);
        }

        let length = buf.len();
        let data = self.descriptors.get(&address).unwrap();
        buf.copy_from_slice(&data[..length as usize]);

        Ok(())
    }

    fn get_string(&mut self, index: u8) -> TransactionResult<String> {
        if !self.strings.contains_key(&index) {
            panic!("String not found: {}", index);
        }

        Ok(self.strings.get(&index).unwrap().clone())
    }
}

impl PhysicalEP0 for FakeEP0 {
    fn set_address(&mut self, _address: u8) -> TransactionResult<()> {
        Ok(())
    }

    fn set_max_packet_size(&mut self, _mps: u8) {}
}
