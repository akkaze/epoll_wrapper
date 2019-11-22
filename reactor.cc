#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <unordered_map>
#include <functional>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable> 
#include <algorithm>
#include <atomic>
#include "reactor.h"
#include "channel.h"
#include "logging.hpp"
#define MAX_EVENTS 100

Reactor::Reactor() {
    epoll_fd = epoll_create(MAX_EVENTS);
    LOG(INFO) << "Created a new poll event" ;
}

void Reactor::ServeForever() {
    loop_thrd = std::unique_ptr<std::thread>(new std::thread([=](){
        for(;;) {
            int ret = Serve();
            if (ret) break;
        }   
    }));
}
void Reactor::Shutdown() {
    int pipe_fd[2];
    pipe(pipe_fd);
    int flags = EPOLLIN;
    this->shutdown_channel_ = new Channel(this, pipe_fd[0], flags); 
    ::write(pipe_fd[1], "shutdown", 9);
}
/**
  * Function which processes the events from epoll_wait and calls the appropriate callbacks
  * @note only process events once if you need to use an event loop use Reactor_loop
*/
int Reactor::Serve() {
    struct epoll_event events[MAX_EVENTS];
    int fds = epoll_wait(this->epoll_fd, events, MAX_EVENTS, this->timeout);
    if (fds == 0)  {
        this->timeout_callback();
    }
    for(int i = 0; i < fds; i++) {
        Channel * value = NULL;
        value = this->channels_[events[i].data.fd];
        if (value) {
            // when data avaliable for read or urgent flag is set
            if ((events[i].events & EPOLLIN) || 
                    (events[i].events & EPOLLPRI)) {
                if (events[i].events & EPOLLIN) {
                    value->cur_event &= EPOLLIN;
                    if (this->shutdown_channel_) {
                        if (events[i].data.fd == this->shutdown_channel_->fd) {
                            return 1;
                        }
                    }
                }
                else {
                    value->cur_event &= EPOLLPRI;
                }
                // accept callback if flag set
                if (value->is_accept) {
                    value->accept_callback();
                }
                // read callback in any case
                if (!(value->is_accept)) {
                    value->read_callback();
                }
            }
            // when write possible
            if (events[i].events & EPOLLOUT) {
                value->cur_event &= EPOLLOUT;
                value->write_callback();
            }
            // shutdown or error
            if ((events[i].events & EPOLLRDHUP) || 
                    (events[i].events & EPOLLERR) || 
                    (events[i].events & EPOLLHUP)) {
                if (events[i].events & EPOLLRDHUP) {
                    value->cur_event &= EPOLLRDHUP;
                }
                else {
                    value->cur_event &= EPOLLERR;
                    value->close_callback();
                }
                return 1;
            }
        } // if
        else {
            return 1;
        }
    } // for
    return 0;
}

