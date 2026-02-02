use crate::poller::{Interest, Poller};
use libc::close;
use std::io;
use thiserror::Error;

#[derive(Debug, Clone, Copy)]
pub struct Event {
    fd: usize,
    interest: Interest,
}

pub struct EventLoop<P: Poller> {
    poller: P,
    fired_io_events: Vec<Option<Event>>,
    io_events: Vec<Option<Box<dyn FnMut(&mut EventLoop<P>, Event) + 'static>>>,
    set_size: usize,
}

#[derive(Error, Debug)]
pub enum EventLoopError {
    #[error(transparent)]
    Io(#[from] io::Error),

    #[error("Too many file descriptors")]
    TooManyFds,

    #[error("Invalid file descriptor")]
    InvalidFd,
}

impl<P: Poller> EventLoop<P> {
    pub fn new(poller: P, set_size: usize) -> Self {
        let mut io_events = Vec::with_capacity(set_size);
        io_events.resize_with(set_size, || None);
        let fired_io_events = vec![None; set_size];
        EventLoop {
            poller,
            io_events: io_events,
            fired_io_events,
            set_size,
        }
    }

    pub fn add_fd<C>(
        &mut self,
        fd: i32,
        interest: Interest,
        callback: C,
    ) -> Result<(), EventLoopError>
    where
        C: FnMut(&mut EventLoop<P>, Event) + 'static,
    {
        if fd < 0 {
            return Err(EventLoopError::InvalidFd);
        }
        if fd as usize >= self.set_size {
            return Err(EventLoopError::TooManyFds);
        }

        self.poller.register(fd, interest);
        self.io_events[fd as usize] = Some(Box::new(callback));

        Ok(())
    }

    pub fn remove_fd(&mut self, fd: i32, interest: Interest) -> Result<(), EventLoopError> {
        if fd < 0 || fd as usize >= self.set_size {
            return Err(EventLoopError::InvalidFd);
        }
        if self.io_events[fd as usize].is_none() {
            return Err(EventLoopError::InvalidFd);
        }

        self.poller.deregister(fd, interest);
        self.io_events[fd as usize] = None;

        Ok(())
    }

    pub fn close_fd(&mut self, fd: i32, interest: Interest) -> Result<(), EventLoopError> {
        self.remove_fd(fd, interest)?;
        unsafe {
            close(fd);
        }

        Ok(())
    }

    pub fn add_timer(&mut self) {
        unimplemented!()
    }

    pub fn run(&mut self) {
        loop {
            self.fired_io_events.fill(None);
            self.poller.poll(&mut self.fired_io_events, None).unwrap();
            for i in 0..self.set_size {
                let maybe_event = self.fired_io_events[i];
                if let Some(event) = maybe_event {
                    self.dispatch(event);
                }
            }
        }
    }

    fn dispatch(&mut self, event: Event) {
        if let Some(mut callback) = self.io_events[event.fd as usize].take() {
            callback(self, event);
            self.io_events[event.fd as usize] = Some(callback);
        }
    }
}

impl<P: Poller> Drop for EventLoop<P> {
    fn drop(&mut self) {
        self.poller.close();
    }
}

impl Event {
    pub fn new(fd: usize, interest: Interest) -> Event {
        Event { interest, fd }
    }
}
