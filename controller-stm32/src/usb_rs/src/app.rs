use super::alloc::format;
use super::alloc::vec;
use super::common::log;
use super::common::sleep_ms;
use super::logger::Logger;
use super::usb::ConfigurationDescriptor;
use super::usb::Descriptor;
use super::usb::DeviceDescriptor;
use super::usb::NewDescriptor;
use super::usb::ParsingContext;
use super::usb::PhysicalEP0;
use super::usb::EP0;
use super::usb::USBEP0;
use super::usb_core::hc::BindedHC;
use super::usb_core::hcd::BindedHcd;
use super::usb_core::hcd::Hcd;

fn run() {
    let mut logger = Logger::new("usb.com", "RUST CODE");

    logger.info("Rust code started");

    {
        let mut hcd = BindedHcd::new();
        hcd.init();
        hcd.wait_device();
        sleep_ms(200);

        hcd.reset_port();
        sleep_ms(100);
    }

    let mut ep0: USBEP0<BindedHC> = USBEP0::new(0);

    logger.trace("\x1b[45m                                        \x1b[0m");
    ep0.set_address(1);
    logger.trace("\x1b[45m                                        \x1b[0m");

    {
        let mut ctx = ParsingContext { ep0: &mut ep0 };

        // get device descriptor
        let desc = DeviceDescriptor::new(&mut ctx, 0).unwrap();

        logger.info(format!("Device Information"));
        logger.info(format!(
            "  Class       : {}-{}-{}",
            desc.usb_class, desc.usb_subclass, desc.usb_proto
        ));
        logger.info(format!("  VID, PID    : {:04X}-{:04X}", desc.vid, desc.pid));
        logger.info(format!("  Configs     : {}", desc.num_configurations));
        logger.info(format!("  Manufacturer: {}", desc.manufacturer));
        logger.info(format!("  Product     : {}", desc.product));
        logger.info(format!("  Serial      : {}", desc.serial));
    }
    logger.trace("\x1b[45m                                        \x1b[0m");

    if false {
        let mut ctx = ParsingContext { ep0: &mut ep0 };

        let total_size = {
            let mut buf = [0; 4];
            ctx.ep0.get_descriptor(0x02, 0, &mut buf, 0x4);
            (buf[3] as u16) << 8 | (buf[2] as u16)
        };

        let mut buf = vec![0; total_size as usize];
        ctx.ep0
            .get_descriptor(0x02, 0, buf.as_mut_slice(), total_size);

        let (_, config) = ConfigurationDescriptor::parse(&mut ctx, buf.as_mut_slice()).unwrap();

        log(format!(
            "Configuration Descriptor[{}: {}]:",
            config.id, config.name
        ));
        log(format!("- Attributes: {}", config.attributes));
        log(format!("- Max Power: {}", config.max_power));
        for interface in config.interfaces {
            log(format!(
                "- Interface [{}: {}]",
                interface.id, interface.name
            ));
            log(format!(
                "  - Kind: {}-{}-{}",
                interface.if_class, interface.if_subclass, interface.if_proto
            ));
            log(format!("  - Alt Setting: {}", interface.alt_setting));
            log(format!("  - Endpoints: {}", interface.num_endpoints));
        }
    }
}

#[no_mangle]
pub extern "C" fn usb_rs_run() {
    run();
}
