#pragma once

#include <cstdint>
#include <cstddef>

namespace HFT {

/**
 * @brief Handles direct-send outbound TCP routing for OUCH 4.2 messages.
 */
class TCPEgressGateway {
public:
    TCPEgressGateway();
    explicit TCPEgressGateway(int fd);

    // Prevent copy/move
    TCPEgressGateway(const TCPEgressGateway&) = delete;
    TCPEgressGateway& operator=(const TCPEgressGateway&) = delete;

    void setSocketFd(int fd);

    /**
     * @brief Send data directly to the TCP socket
     * @param buffer The byte buffer to send
     * @param length Number of bytes
     * @return true if all bytes were sent, false otherwise
     */
    bool send(const uint8_t* buffer, size_t length);

private:
    int fd_;
};

} // namespace HFT
