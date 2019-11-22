#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include "poll.h"
#include "poll_element.h"
#include "logging.hpp"
#define BUFFSIZE 1024

int main() {
    // create a TCP socket, bind and listen
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in svr_addr;
    memset(&svr_addr, 0 , sizeof(svr_addr));
    svr_addr.sin_family = AF_INET;
    svr_addr.sin_addr.s_addr = htons(INADDR_ANY);
    svr_addr.sin_port = htons(9999);
    bind(sock, (struct sockaddr *) &svr_addr, sizeof(svr_addr));
    listen(sock, 10);
    int opt = 1;
    setsockopt(sock,SOL_SOCKET,SO_REUSEPORT,&opt,sizeof(opt));
    fcntl(sock, F_SETFL, O_NONBLOCK);

    // create a poll event object, with time out of 1 sec
    poll_event *pe = new poll_event();
    pe->poll_event_loop();
    int flags = EPOLLIN;
    poll_event_element *p = new poll_event_element(pe, sock, flags);

    p->is_accept = true;
    std::vector<poll_event_element*> clients;
    std::vector<std::thread> read_thrs;
    // enable accept callback
    for (int j = 0; j < 2;  j++) {
        poll_event_element* c;
        p->accept(&c);
        clients.emplace_back(c); 
        std::thread read_th([=]() {
            for (int i = 0; i < 20; i++) {
              char* buf = new char[i + 1];
              memset(buf, 0, i+1);
              c->read(i + 1, buf);
              delete[] buf;
            }
        });
        read_thrs.emplace_back(std::move(read_th));
    }
    for (auto& read_th : read_thrs) {
        read_th.join();
    }
    for (auto client : clients) {
        delete client;
    }
    // start the event loop
    delete p;
    delete pe;
    return 0;
}

