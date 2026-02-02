use crate::{
    event_loop::Event,
    poller::{Interest, Poller, PollerError},
};
use libc::{self, EV_ADD, EV_DELETE, EV_ENABLE, EVFILT_READ, EVFILT_WRITE, close, kevent, kqueue};
use std::{mem, ptr};

pub struct KqueuePoller {
    fd: i32,
    change_list: Vec<kevent>,
    event_list: Vec<kevent>,
}

impl KqueuePoller {
    pub fn new() -> Result<Self, PollerError> {
        let fd = unsafe { kqueue() };

        if fd == -1 {
            return Err(PollerError::Init);
        }

        Ok(KqueuePoller {
            fd,
            change_list: Vec::new(),
            event_list: vec![unsafe { mem::zeroed::<kevent>() }; 128],
        })
    }
}

impl Poller for KqueuePoller {
    fn register(&mut self, fd: i32, interest: crate::poller::Interest) {
        match interest {
            Interest::READ => {
                self.change_list.push(kevent {
                    ident: fd as usize,
                    filter: EVFILT_READ,
                    flags: EV_ADD | EV_ENABLE,
                    fflags: 0,
                    data: 0,
                    udata: ptr::null_mut(),
                });
            }
            Interest::WRITE => {
                self.change_list.push(kevent {
                    ident: fd as usize,
                    filter: EVFILT_WRITE,
                    flags: EV_ADD | EV_ENABLE,
                    fflags: 0,
                    data: 0,
                    udata: ptr::null_mut(),
                });
            }
            Interest::READ_WRITE => {
                self.change_list.push(kevent {
                    ident: fd as usize,
                    filter: EVFILT_READ,
                    flags: EV_ADD | EV_ENABLE,
                    fflags: 0,
                    data: 0,
                    udata: ptr::null_mut(),
                });
                self.change_list.push(kevent {
                    ident: fd as usize,
                    filter: EVFILT_WRITE,
                    flags: EV_ADD | EV_ENABLE,
                    fflags: 0,
                    data: 0,
                    udata: ptr::null_mut(),
                });
            }
            _ => unreachable!(),
        }
    }

    fn deregister(&mut self, fd: i32, interest: crate::poller::Interest) {
        match interest {
            Interest::READ => {
                self.change_list.push(kevent {
                    ident: fd as usize,
                    filter: EVFILT_READ,
                    flags: EV_DELETE,
                    fflags: 0,
                    data: 0,
                    udata: ptr::null_mut(),
                });
            }
            Interest::WRITE => {
                self.change_list.push(kevent {
                    ident: fd as usize,
                    filter: EVFILT_WRITE,
                    flags: EV_DELETE,
                    fflags: 0,
                    data: 0,
                    udata: ptr::null_mut(),
                });
            }
            Interest::READ_WRITE => {
                self.change_list.push(kevent {
                    ident: fd as usize,
                    filter: EVFILT_READ,
                    flags: EV_DELETE,
                    fflags: 0,
                    data: 0,
                    udata: ptr::null_mut(),
                });
                self.change_list.push(kevent {
                    ident: fd as usize,
                    filter: EVFILT_WRITE,
                    flags: EV_DELETE,
                    fflags: 0,
                    data: 0,
                    udata: ptr::null_mut(),
                });
            }
            _ => unreachable!(),
        }
    }

    fn poll(
        &mut self,
        out: &mut Vec<Option<Event>>,
        timeout: Option<std::time::Duration>,
    ) -> Result<(), PollerError> {
        let nev = unsafe {
            kevent(
                self.fd,
                self.change_list.as_ptr(),
                self.change_list.len() as i32,
                self.event_list.as_mut_ptr(),
                self.event_list.len() as i32,
                ptr::null(), // FIXME: pass the timeout
            )
        };

        if nev == -1 {
            return Err(PollerError::Poll);
        }

        self.change_list.clear();

        if nev > 0 {
            for ev in &self.event_list[..nev as usize] {
                let fd = ev.ident;
                if ev.filter == EVFILT_READ {
                    out[fd] = Some(Event::new(fd, Interest::READ))
                } else if ev.filter == EVFILT_WRITE {
                    out[fd] = Some(Event::new(fd, Interest::WRITE))
                }
            }
        }

        Ok(())
    }

    fn close(&mut self) {
        unsafe {
            close(self.fd);
        }
    }
}
