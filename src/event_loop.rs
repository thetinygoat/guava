use crate::{
    poller::{Interest, Poller, Token},
    pollers,
};
use std::{collections::HashMap, io, mem};
use thiserror::Error;

#[derive(Debug, Clone, Copy)]
pub struct Event {
    token: Token,
    read: bool,
    write: bool,
}

pub struct EventLoop<P: Poller> {
    poller: P,
    fds: Vec<i32>,
    events: Vec<Event>,
    handlers: HashMap<Token, Box<dyn FnMut(&mut EventLoop<P>, Event) + 'static>>,
}

#[derive(Error, Debug)]
pub enum EventLoopError {
    #[error(transparent)]
    Io(#[from] io::Error),
}

impl<P: Poller> EventLoop<P> {
    pub fn new(poller: P) -> Self {
        EventLoop {
            poller,
            fds: Vec::with_capacity(1024),
            events: Vec::with_capacity(256),
            handlers: HashMap::with_capacity(1024),
        }
    }

    pub fn add_fd<F>(
        &mut self,
        fd: i32,
        interest: Interest,
        handler: F,
    ) -> Result<Token, EventLoopError>
    where
        F: FnMut(&mut EventLoop<P>, Event) + 'static,
    {
        let token = Token(self.handlers.len());
        self.poller.register(fd, token, interest);
        self.fds.push(fd);
        Ok(token)
    }

    pub fn add_timer(&mut self) {
        unimplemented!()
    }

    pub fn run(&mut self) {
        loop {
            self.poller.poll(&mut self.events, None);
            let events = mem::take(&mut self.events);
            for event in events {
                self.dispatch(event);
            }
        }
    }

    fn dispatch(&mut self, event: Event) {
        if let Some(mut handler) = self.handlers.remove(&event.token) {
            handler(self, event);
            self.handlers.insert(event.token, handler);
        }
    }
}
