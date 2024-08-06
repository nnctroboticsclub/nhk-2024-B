pub trait Hcd {
    fn new() -> Self;
    fn init(&mut self);
    fn wait_device(&mut self);
    fn reset_port(&mut self);
}
