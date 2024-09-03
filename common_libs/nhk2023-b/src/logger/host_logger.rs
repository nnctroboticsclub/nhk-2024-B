use alloc::{borrow::Cow, string::String};

pub enum LoggerLevel {
    Info,
    Error,
    Debug,
    Trace,
    Verbose,
}

impl Into<&str> for LoggerLevel {
    fn into(self) -> &'static str {
        match self {
            LoggerLevel::Info => "\x1b[1;31mE\x1b[m",
            LoggerLevel::Error => "\x1b[1;32mI\x1b[m",
            LoggerLevel::Debug => "\x1b[1;34mV\x1b[m",
            LoggerLevel::Trace => "\x1b[1;36mD\x1b[m",
            LoggerLevel::Verbose => "\x1b[1;35mT\x1b[m",
        }
    }
}

pub struct Logger {
    _id: String,
    header: String,
}

impl Logger {
    pub fn new<'a, 'b, S, S2>(id: S, header: S2) -> Logger
    where
        S: Into<Cow<'a, str>>,
        S2: Into<Cow<'b, str>>,
    {
        let id: Cow<'a, str> = id.into();
        let header: Cow<'b, str> = header.into();

        let id = id.into_owned();
        let header = header.into_owned();

        Logger { _id: id, header }
    }

    fn log(&mut self, level: LoggerLevel, message: String) {
        let level: &str = level.into();

        println!("{} [{}] {}", level, self.header, message);
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
        // format:
        // 01020304 05060708 090A0B0C 0D0E0F10
        // 01020304 05060708 090A0B0C 0D0E0F10

        // buffer: buf (String)
        // input: buffer, length

        let mut s = String::new();
        for i in 0..length {
            if i % 16 == 0 {
                s.push('\n');
            } else if i % 8 == 0 {
                s.push(' ');
            }

            s.push_str(&format!("{:02X}", buffer[i as usize]));
        }

        self.log(level, s);
    }
}
