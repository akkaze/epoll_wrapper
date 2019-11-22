#include <stdio.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include "poll.h"
#include "logging.hpp"

void gen_random(std::string& s) {
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    srand(time(0));
    for (int i = 0; i < s.size(); ++i) {
        s[i] = alphanum[rand() % (sizeof(alphanum) - 1)];
    }

}

void write_loop(poll_event_element* p) {
    std::string s(200, '\0');
    gen_random(s);
    for (int i = 0; i < 20; i++) {
      //char buf[BUFSIZE] = "hello world";
      std::string ss = s.substr(0, i + 1);
      p->write(i + 1, const_cast<char*>(ss.c_str()));
//      sleep(1);
      LOG(INFO) << ss.c_str();
    }
}

int main() {
    // create a TCP socket, bind and listen
    int sockfd;
    char host[] = "127.0.0.1";
    int port = 9999;
/* ============sockfd============ */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(host);
    servaddr.sin_port = htons(port);
/* ============connect============ */
    connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    // create a poll event object, with time out of 1 sec
    poll_event *pe = new poll_event();
    pe->poll_event_loop();
    // set timeout callback

    poll_event_element* p = new poll_event_element(pe, sockfd, EPOLLOUT);
    write_loop(p);
    
    delete pe;
    delete p;
    return 0;
}

