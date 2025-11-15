extern crate alloc;

use alloc::{borrow::Cow, ffi::CString, string::String};

use super::binding_logger::{
    robotics_logger_GenericLogger, robotics_logger_GenericLogger_Log, robotics_logger_core_Level,
    robotics_logger_core_Level_kDebug, robotics_logger_core_Level_kError,
    robotics_logger_core_Level_kInfo, robotics_logger_core_Level_kTrace,
    robotics_logger_core_Level_kVerbose,
};

pub enum LoggerLevel {
    Info,
    Error,
    Debug,
    Trace,
    Verbose,
}

impl Into<robotics_logger_core_Level> for LoggerLevel {
    fn into(self) -> robotics_logger_core_Level {
        match self {
            LoggerLevel::Info => robotics_logger_core_Level_kInfo,
            LoggerLevel::Error => robotics_logger_core_Level_kError,
            LoggerLevel::Debug => robotics_logger_core_Level_kDebug,
            LoggerLevel::Trace => robotics_logger_core_Level_kTrace,
            LoggerLevel::Verbose => robotics_logger_core_Level_kVerbose,
        }
    }
}

pub struct Logger {
    logger: robotics_logger_GenericLogger,
    id: CString,
    header: CString,
}

impl Logger {
    pub fn new<'a, 'b, S: Into<Cow<'a, str>>, S2: Into<Cow<'b, str>>>(id: S, header: S2) -> Logger {
        let id: Cow<'a, str> = id.into();
        let header: Cow<'b, str> = header.into();

        let id = CString::new(id.into_owned()).unwrap();
        let header = CString::new(header.into_owned()).unwrap();

        Logger {
            logger: unsafe { robotics_logger_GenericLogger::new(id.as_ptr(), header.as_ptr()) },
            id,
            header,
        }
    }

    fn log(&mut self, level: LoggerLevel, message: String) {
        let message_length = message.len();

        let c_message = CString::new(message).unwrap();
        let c_message_ptr = c_message.as_ptr();

        unsafe {
            robotics_logger_GenericLogger_Log(
                &mut self.logger,
                level.into(),
                "%*s\0".as_ptr() as *const u8,
                message_length as i32,
                c_message_ptr as *const i8,
            );
        }
    }

    pub fn info<'a, S: Into<Cow<'a, str>>>(&mut self, message: S) {
        let s: Cow<'a, str> = message.into();
        let _: &str = &s;
        let s: String = s.into_owned();
        self.log(LoggerLevel::Info, s);
    }

    pub fn error<'a, S: Into<Cow<'a, str>>>(&mut self, message: S) {
        let s: Cow<'a, str> = message.into();
        let _: &str = &s;
        let s: String = s.into_owned();
        self.log(LoggerLevel::Error, s);
    }

    pub fn debug<'a, S: Into<Cow<'a, str>>>(&mut self, message: S) {
        let s: Cow<'a, str> = message.into();
        let _: &str = &s;
        let s: String = s.into_owned();
        self.log(LoggerLevel::Debug, s);
    }

    pub fn trace<'a, S: Into<Cow<'a, str>>>(&mut self, message: S) {
        let s: Cow<'a, str> = message.into();
        let _: &str = &s;
        let s: String = s.into_owned();
        self.log(LoggerLevel::Trace, s);
    }

    pub fn verbose<'a, S: Into<Cow<'a, str>>>(&mut self, message: S) {
        let s: Cow<'a, str> = message.into();
        let _: &str = &s;
        let s: String = s.into_owned();
        self.log(LoggerLevel::Verbose, s);
    }

    pub fn hex(&mut self, level: LoggerLevel, buffer: &[u8], length: i32) {
        unsafe {
            self.logger
                .Hex(level.into(), buffer.as_ptr(), length as usize);
        }
    }
}
