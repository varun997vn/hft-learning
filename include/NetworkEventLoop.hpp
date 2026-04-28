#pragma once

#include <sys/epoll.h>
#include <functional>
#include <vector>
#include <string>
#include <cstdint>

namespace HFT {

// Callback function type for when a socket is ready to read
using ReadCallback = std::function<void(int fd)>;

class NetworkEventLoop {
public:
    NetworkEventLoop();
    ~NetworkEventLoop();

    // Prevent copy/move
    NetworkEventLoop(const NetworkEventLoop&) = delete;
    NetworkEventLoop& operator=(const NetworkEventLoop&) = delete;

    /**
     * @brief Initialize the epoll instance
     * @return true if successful
     */
    bool init();

    /**
     * @brief Run the event loop indefinitely. Blocks current thread.
     */
    void run();

    /**
     * @brief Stop the event loop safely.
     */
    void stop();

    /**
     * @brief Create a UDP Multicast listening socket and add to epoll
     * @param mcast_ip The multicast group IP address (e.g. "233.54.12.211")
     * @param port The port to listen on
     * @param callback The function to execute when data is ready
     * @return The socket file descriptor, or -1 on failure
     */
    int addUDPMulticastListener(const std::string& mcast_ip, uint16_t port, ReadCallback callback);

    /**
     * @brief Create a TCP listening socket (e.g. for OUCH server)
     * @param port The port to bind to
     * @param callback The function to execute when a new connection arrives
     * @return The socket file descriptor, or -1 on failure
     */
    int addTCPListener(uint16_t port, ReadCallback callback);

    /**
     * @brief Create a TCP client socket and connect to a remote host
     * @param ip The IP to connect to
     * @param port The port to connect to
     * @param callback The function to execute when data is ready to read
     * @return The socket file descriptor, or -1 on failure
     */
    int addTCPClient(const std::string& ip, uint16_t port, ReadCallback callback);

private:
    int epoll_fd_;
    bool running_;
    
    // We maintain a direct mapping of fd -> callback for O(1) dispatching.
    // In a real HFT system we might just use a pre-allocated flat array
    // since fd integers are small and contiguous. We'll use a vector for simplicity and fast lookup.
    std::vector<ReadCallback> fd_callbacks_;
    
    void setNonBlocking(int fd);
    bool registerFd(int fd, ReadCallback callback);
};

} // namespace HFT
