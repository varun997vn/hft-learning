#include "../include/NetworkEventLoop.hpp"
#include "../include/AsyncLogger.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>

#define MAX_EVENTS 64

namespace HFT {

NetworkEventLoop::NetworkEventLoop() : epoll_fd_(-1), running_(false) {
    // Pre-allocate enough space for standard FDs
    fd_callbacks_.resize(1024);
}

NetworkEventLoop::~NetworkEventLoop() {
    stop();
    if (epoll_fd_ != -1) {
        close(epoll_fd_);
    }
}

bool NetworkEventLoop::init() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ == -1) {
        AsyncLogger::getInstance().log(LogLevel::ERROR, "NetworkEventLoop: epoll_create1 failed");
        return false;
    }
    return true;
}

void NetworkEventLoop::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

bool NetworkEventLoop::registerFd(int fd, ReadCallback callback) {
    if (fd >= static_cast<int>(fd_callbacks_.size())) {
        fd_callbacks_.resize(fd * 2);
    }
    
    setNonBlocking(fd);
    fd_callbacks_[fd] = std::move(callback);

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; // Edge-triggered read
    ev.data.fd = fd;

    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) == -1) {
        AsyncLogger::getInstance().log(LogLevel::ERROR, "NetworkEventLoop: epoll_ctl failed for fd %d", fd);
        return false;
    }

    return true;
}

int NetworkEventLoop::addUDPMulticastListener(const std::string& mcast_ip, uint16_t port, ReadCallback callback) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
        AsyncLogger::getInstance().log(LogLevel::ERROR, "NetworkEventLoop: UDP socket creation failed");
        return -1;
    }

    // Allow multiple sockets to use the same PORT number
    int reuse = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        AsyncLogger::getInstance().log(LogLevel::WARN, "NetworkEventLoop: Setting SO_REUSEADDR failed");
    }

    struct sockaddr_in local_addr;
    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(port);
    local_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(sock, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        AsyncLogger::getInstance().log(LogLevel::ERROR, "NetworkEventLoop: UDP bind failed on port %u", port);
        close(sock);
        return -1;
    }

    // Join the multicast group
    struct ip_mreq group;
    group.imr_multiaddr.s_addr = inet_addr(mcast_ip.c_str());
    group.imr_interface.s_addr = INADDR_ANY;
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, &group, sizeof(group)) < 0) {
        AsyncLogger::getInstance().log(LogLevel::ERROR, "NetworkEventLoop: Adding multicast group %s failed", mcast_ip.c_str());
        close(sock);
        return -1;
    }

    if (!registerFd(sock, callback)) {
        close(sock);
        return -1;
    }

    AsyncLogger::getInstance().log(LogLevel::INFO, "NetworkEventLoop: Listening on UDP multicast %s:%u", mcast_ip.c_str(), port);
    return sock;
}

int NetworkEventLoop::addTCPListener(uint16_t port, ReadCallback callback) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        AsyncLogger::getInstance().log(LogLevel::ERROR, "NetworkEventLoop: TCP socket creation failed");
        return -1;
    }

    int reuse = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        AsyncLogger::getInstance().log(LogLevel::ERROR, "NetworkEventLoop: TCP bind failed on port %u", port);
        close(sock);
        return -1;
    }

    if (listen(sock, SOMAXCONN) < 0) {
        AsyncLogger::getInstance().log(LogLevel::ERROR, "NetworkEventLoop: TCP listen failed");
        close(sock);
        return -1;
    }

    if (!registerFd(sock, callback)) {
        close(sock);
        return -1;
    }

    AsyncLogger::getInstance().log(LogLevel::INFO, "NetworkEventLoop: Listening on TCP port %u", port);
    return sock;
}

int NetworkEventLoop::addTCPClient(const std::string& ip, uint16_t port, ReadCallback callback) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        AsyncLogger::getInstance().log(LogLevel::ERROR, "NetworkEventLoop: TCP client socket creation failed");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) <= 0) {
        AsyncLogger::getInstance().log(LogLevel::ERROR, "NetworkEventLoop: Invalid IP address for TCP client");
        close(sock);
        return -1;
    }

    // Set non-blocking before connect to avoid blocking the thread
    setNonBlocking(sock);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        if (errno != EINPROGRESS) {
            AsyncLogger::getInstance().log(LogLevel::ERROR, "NetworkEventLoop: TCP client connect failed");
            close(sock);
            return -1;
        }
    }

    if (!registerFd(sock, std::move(callback))) {
        close(sock);
        return -1;
    }

    AsyncLogger::getInstance().log(LogLevel::INFO, "NetworkEventLoop: Connecting to TCP %s:%u", ip.c_str(), port);
    return sock;
}

void NetworkEventLoop::run() {
    struct epoll_event events[MAX_EVENTS];
    running_ = true;
    
    AsyncLogger::getInstance().log(LogLevel::INFO, "NetworkEventLoop: Starting event loop");

    while (running_) {
        // -1 means block indefinitely until an event occurs
        int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);
        
        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            
            // O(1) branchless callback dispatch
            if (fd >= 0 && fd < static_cast<int>(fd_callbacks_.size()) && fd_callbacks_[fd]) {
                fd_callbacks_[fd](fd);
            }
        }
    }
    
    AsyncLogger::getInstance().log(LogLevel::INFO, "NetworkEventLoop: Event loop stopped");
}

void NetworkEventLoop::stop() {
    running_ = false;
}

} // namespace HFT
