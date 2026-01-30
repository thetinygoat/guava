use crate::poller::Poller;
use libc::{self, kqueue};

pub struct KqueuePoller {
    fd: i32,
}

impl KqueuePoller {
    pub fn new() -> Self {
        let fd = unsafe { kqueue() };

        if fd == -1 {
            unimplemented!();
        }

        KqueuePoller { fd }
    }
}

impl Poller for KqueuePoller {
    fn register(
        &mut self,
        fd: i32,
        token: crate::poller::Token,
        interest: crate::poller::Interest,
    ) {
    }

    fn deregister(&mut self, fd: i32) {}

    fn poll(
        &mut self,
        out: &mut Vec<crate::event_loop::Event>,
        timeout: Option<std::time::Duration>,
    ) {
    }
}
