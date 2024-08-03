use crate::{
    common::log,
    usb::{ParsingContext, PhysicalEP0, UsbString, EP0},
};
use alloc::string::String;
use nom::{
    bytes::complete::tag,
    number::complete::{le_u16, le_u8},
    IResult,
};

use super::{descriptor::Descriptor, new_descriptor::NewDescriptor};

pub struct DeviceDescriptor {
    // length
    // type
    pub bcd_usb: u16,
    pub usb_class: u8,
    pub usb_subclass: u8,
    pub usb_proto: u8,
    pub mps: u8,

    pub vid: u16,
    pub pid: u16,
    pub bcd: u16,
    pub manufacturer: String,
    pub product: String,

    pub serial: String,
    pub num_configurations: u8,
}

impl Descriptor for DeviceDescriptor {
    fn get_type() -> u8 {
        1
    }

    fn get_length() -> Option<usize> {
        Some(12)
    }

    fn parse<'a>(ctx: &mut ParsingContext<impl EP0>, input: &'a [u8]) -> IResult<&'a [u8], Self> {
        let (input, _) = le_u8(input)?;
        let (input, _) = tag("\x01")(input)?; // type
        let (input, bcd_usb) = le_u16(input)?;
        let (input, usb_class) = le_u8(input)?;
        let (input, usb_subclass) = le_u8(input)?;
        let (input, usb_proto) = le_u8(input)?;
        let (input, mps) = le_u8(input)?;
        let (input, vid) = le_u16(input)?;
        let (input, pid) = le_u16(input)?;
        let (input, bcd) = le_u16(input)?;
        let (input, i_manufacturer) = le_u8(input)?;
        let (input, i_product) = le_u8(input)?;
        let (input, i_serial) = le_u8(input)?;
        let (input, num_configurations) = le_u8(input)?;

        let manufacturer = UsbString::new(i_manufacturer).read(ctx);
        let product = UsbString::new(i_product).read(ctx);
        let serial = UsbString::new(i_serial).read(ctx);

        Ok((
            input,
            DeviceDescriptor {
                bcd_usb,
                usb_class,
                usb_subclass,
                usb_proto,
                mps,
                vid,
                pid,
                bcd,
                manufacturer,
                product,
                serial,
                num_configurations,
            },
        ))
    }
}

impl<T: EP0 + PhysicalEP0> NewDescriptor<T> for DeviceDescriptor {
    fn new(ctx: &mut ParsingContext<T>, index: u8) -> Option<Self> {
        log("\x1b[46m                                        \x1b[0m");
        let mps = {
            let buf = &mut [0; 8];
            ctx.ep0.get_descriptor(1, index, buf, 8);
            buf[7]
        };
        ctx.ep0.set_max_packet_size(mps);
        log("\x1b[46m                                        \x1b[0m");

        // usually object construction
        let buf = &mut [0; 0x12];
        ctx.ep0.get_descriptor(1, index, buf, 0x12);
        log("\x1b[46m                                        \x1b[0m");

        Self::parse(ctx, buf).map(|x| x.1).ok()
    }
}

#[cfg(not(target_os = "none"))]
#[cfg(test)]
mod tests {
    use crate::usb::{
        fake_ep0::{DescriptorType, FakeEP0},
        NewDescriptor,
    };

    #[test]
    fn test_device_descriptor() {
        let buf = [
            0x12, 0x01, 0x10, 0x01, 0x00, 0x01, 0x02, 0x08, //
            0x22, 0x11, 0x44, 0x33, 0x66, 0x55, 0x01, 0x02, //
            0x03, 0x01,
        ];

        let mut ctx = super::ParsingContext {
            ep0: &mut FakeEP0::default(),
        };

        ctx.ep0
            .add_descriptor(DescriptorType::Device, 0, buf.to_vec());

        ctx.ep0.add_string(1, "Manufacturer".to_string());
        ctx.ep0.add_string(2, "Product".to_string());
        ctx.ep0.add_string(3, "Serial".to_string());

        let desc = super::DeviceDescriptor::new(&mut ctx, 0).unwrap();

        assert_eq!(desc.bcd_usb, 0x0110);
        assert_eq!(desc.usb_class, 0x00);
        assert_eq!(desc.usb_subclass, 0x01);
        assert_eq!(desc.usb_proto, 0x02);
        assert_eq!(desc.mps, 0x08);
        assert_eq!(desc.vid, 0x1122);
        assert_eq!(desc.pid, 0x3344);
        assert_eq!(desc.bcd, 0x5566);
        assert_eq!(desc.manufacturer, "Manufacturer");
        assert_eq!(desc.product, "Product");
        assert_eq!(desc.serial, "Serial");
        assert_eq!(desc.num_configurations, 0x01);
    }
}
