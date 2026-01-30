use crate::event_loop::Event;
use std::{i32, time::Duration};

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Token(pub usize);

#[derive(Debug)]
pub struct Interest(u8);

impl Interest {
    pub const READ: Self = Self(0b01);
    pub const WRITE: Self = Self(0b10);
    pub const READ_WRITE: Self = Self(0b11);
}

pub trait Poller {
    fn register(&mut self, fd: i32, token: Token, interest: Interest);
    fn deregister(&mut self, fd: i32);
    fn poll(&mut self, out: &mut Vec<Event>, timeout: Option<Duration>);
}
