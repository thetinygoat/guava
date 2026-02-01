use crate::event_loop::Event;
use std::{i32, time::Duration};
use thiserror::Error;

#[derive(Debug, Clone, Copy, PartialEq, Eq, Hash)]
pub struct Token(pub usize);

#[derive(Debug, PartialEq, Eq)]
pub struct Interest(u8);

impl Interest {
    pub const READ: Self = Self(0b01);
    pub const WRITE: Self = Self(0b10);
    pub const READ_WRITE: Self = Self(0b11);
}

pub trait Poller {
    fn register(&mut self, fd: i32, token: Token, interest: Interest);
    fn deregister(&mut self, fd: i32, interest: Interest);
    fn poll(&mut self, out: &mut Vec<Event>, timeout: Option<Duration>) -> Result<(), PollerError>;
    fn close(&mut self);
}

#[derive(Debug, Error)]
pub enum PollerError {
    #[error("Failed to initialize poller")]
    Init,

    #[error("Failed to poll for events")]
    Poll,
}
