#[cfg(target_os = "none")]
mod binding_logger;

#[cfg(target_os = "none")]
mod logger;

#[cfg(not(target_os = "none"))]
mod host_logger;

#[cfg(target_os = "none")]
pub use logger::Logger;

#[cfg(not(target_os = "none"))]
pub use host_logger::Logger;

pub use logger::LoggerLevel;
