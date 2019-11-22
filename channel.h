#pragma once
#include <atomic>
//#include "reactor.h"
#include "logging.hpp"
/**
 * @struct Channel "poll.h"
 * @brief a poll event element containing callbacks, user data and flags
 */
#define BUFSIZE 1024
struct Reactor;
struct Channel {
    Channel(Reactor* pe, uint32_t flags);
    Channel(Reactor* pe, uint32_t fd, uint32_t flags);
    virtual ~Channel() {
        Release();
    }
    /**
      * Function to add a file descriptor to the event poll obeject
      * @note if add is performed on an fd already in Reactor, the flags are updated in the existing object
      * @param Reactor poll event object which fd has to be added
      * @param flags events flags from epoll
    */
    int Create(uint32_t flags);
    int Connect(const std::string& hostname, const int32_t& port);
    int Listen(const int32_t& port, const size_t& backlog);
    /**
      * Function to remove a poll event element from the given Reactor object
      * @param Reactor poll event object from which fd has to be removed
      * @param fd file descriptor which has to be removed
    */
    int Release();
    void read_callback ();
    void write_callback ();
    
    void accept_callback();
    
    void close_callback();
    void accept(Channel** add);
    void write(int count, char* data);
    void read(int count, char* data);
    int fd;
    //std::vector<char> rd_buf;
    //std::vector<char> wr_buf;
    char* rd_buf;
    char* wr_buf;
    int rd_size;
    int wr_size;
    /** the event for which callback was initiated */
    uint32_t cur_event;
    std::atomic<bool> rd_done{true};
    std::atomic<bool> wr_done{true};
    std::atomic<bool> accept_done{false};
    size_t rd_rem;
    size_t wr_rem;
    std::vector<int32_t> listen_fds;
    /** only used to enable accept and listen callbacks */
    bool is_accept;
    Reactor* pe;
}; 
