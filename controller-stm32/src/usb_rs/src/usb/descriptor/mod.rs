mod configuration_descriptor;
mod descriptor;
mod device_descriptor;
mod endpoint_descriptor;
mod hid_class_descriptor;
mod hid_descriptor;
mod interface_descriptor;
mod new_descriptor;

pub use configuration_descriptor::ConfigurationDescriptor;
pub use descriptor::Descriptor;
pub use device_descriptor::DeviceDescriptor;
pub use endpoint_descriptor::EndpointDescriptor;
pub use hid_descriptor::HIDDescriptor;
pub use interface_descriptor::InterfaceDescriptor;
pub use new_descriptor::NewDescriptor;
