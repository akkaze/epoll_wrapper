#include <sys/epoll.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <cstring>
#include "reactor.h"
#include "channel.h"
#include "logging.hpp"
Channel::Channel(Reactor* pe, uint32_t flags) {
    this->pe = pe;
    this->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    Create(flags);
}

Channel::Channel(Reactor* pe, uint32_t fd, uint32_t flags) {
    this->pe = pe;
    this->fd = fd;
    Create(flags);
}

int Channel::Create(uint32_t flags) {
    this->pe->channels_[fd] = this;
    LOG(INFO) << "Added fd " << fd;
    struct epoll_event ev;
    memset(&ev, 0, sizeof(struct epoll_event));
    ev.data.fd = fd;
    ev.events |= flags;
    return epoll_ctl(pe->epoll_fd, EPOLL_CTL_ADD, fd, &ev);
}
int Channel::Connect(const std::string& hostname, const int32_t& port) {
    struct sockaddr_in servaddr;
    std::memset(&servaddr, sizeof(servaddr), 0);
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(hostname.c_str());
    servaddr.sin_port = htons(port);
    if (::connect(this->fd, (struct sockaddr*)&servaddr, 
                sizeof(servaddr)) != 0) { 
        LOG(INFO) << "Fail to connect to host " << hostname << 
            " port " << port << " :" << strerror(errno);
    }
    fcntl(this->fd, F_SETFL, O_NONBLOCK);
    return 0;
}

int Channel::Listen(const int32_t& port, const size_t& backlog = 1024) {
    struct sockaddr_in own_addr;
    std::memset(&own_addr, 0 , sizeof(own_addr));
    own_addr.sin_family = AF_INET;
    own_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    own_addr.sin_port = htons(port);
    if(::bind(this->fd, (struct sockaddr *) &own_addr, sizeof(own_addr)) != 0) {
        LOG(INFO) << "Fail to bind on port " << port << " :" << strerror(errno);
    };
    if (::listen(this->fd, backlog) != 0) {
        LOG(INFO) << "Fail to listen on port " << port << " :" << strerror(errno);
    }
    int32_t opt = 1;
    ::setsockopt(this->fd, SOL_SOCKET, SO_REUSEADDR,
            &opt,sizeof(opt));
    return 0;
}
/**
  * Function to remove a poll event element from the given Reactor object
  * @param Reactor poll event object from which fd has to be removed
  * @param fd file descriptor which has to be removed
*/
int Channel::Release() {
    Channel *elem = NULL;
    elem = this->pe->channels_[fd];
    if(elem) {
	    pe->channels_[fd] = NULL;   
   	    close(fd);
    	epoll_ctl(pe->epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    }
    return 0;
}
void Channel::read_callback () {
    if (this->rd_done == false) {
        int val = 0;
        while( (val = ::read(this->fd, this->rd_buf + 
                    this->rd_size - this->rd_rem, 
                    this->rd_rem)) > 0) {            
            this->rd_rem -= val;
        }
        if (this->rd_rem == 0) {
            this->rd_done = true;
        }
    }
}
void Channel::write_callback () {
    if (this->wr_done == false) {
        int val = 0;
        while( (val = ::write(this->fd, this->wr_buf + 
                    this->wr_size - this->wr_rem, 
                    this->wr_rem)) > 0) {            
            this->wr_rem -= val;
        }
        if (this->wr_rem == 0) {
            this->wr_done = true;
        }
    }
}


void Channel::accept_callback() {
    if (!this->accept_done) {
        LOG(INFO) << "in accept_cb";
        // accept the connection 
        struct sockaddr_in clt_addr;
        socklen_t clt_len = sizeof(clt_addr);
        int listenfd = ::accept(this->fd, (struct sockaddr*) &clt_addr, &clt_len);
        this->listen_fds.push_back(listenfd);
        fcntl(listenfd, F_SETFL, O_NONBLOCK);
        // set flags to check 
        uint32_t flags = EPOLLIN;
        Channel *p = new  Channel(pe, listenfd, flags);
        pe->last_added = p;
        this->accept_done = true;
    }
}
void Channel::close_callback() {}
void Channel::accept(Channel** add) {
    while(!this->accept_done) {}
    *add = pe->last_added;
    this->accept_done = false;
}
void Channel::write(int count, char* data) {
    while(!this->wr_done) {}
    this->wr_buf = data;
    this->wr_size = count;
    this->wr_rem = count;
    this->wr_done = false;
}
void Channel::read(int count, char* data) {
    this->rd_size = count;
    this->rd_rem = count;
    this->rd_done = false;
    this->rd_buf = data;
    while (!this->rd_done) {}
}
