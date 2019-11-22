#pragma once
#include <unordered_map>
#include <functional>
#include <vector>
#include <thread>
#include <algorithm>
#include "channel.h"
#include "logging.hpp"
#define MAX_EVENTS 100

/**
 * @struct Reactor "reactor.h"
 * @brief reactor object 
 */
struct Reactor {
    Reactor();
    /** timeout duration */
    size_t timeout;
    /** epoll file descriptor*/
    int32_t epoll_fd;
    std::unordered_map<int32_t, Channel*> channels_;
    Channel* last_added;
    ~Reactor() {
        this->Shutdown();
        loop_thrd->join();
        delete shutdown_channel_;
    }
    void Shutdown();
    std::unique_ptr<std::thread> loop_thrd;
    void timeout_callback() {
//        LOG(INFO) << "timeout";
    }
    void ServeForever();
    /**
      * Function which processes the events from epoll_wait and calls the appropriate callbacks
      * @note only process events once if you need to use an event loop use Reactor_loop
    */
    int Serve();
    Channel* shutdown_channel_;
};

