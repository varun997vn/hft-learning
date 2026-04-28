#include "../include/TCPEgress.hpp"
#include "../include/AsyncLogger.hpp"
#include <sys/socket.h>
#include <cerrno>

namespace HFT {

TCPEgressGateway::TCPEgressGateway() : fd_(-1) {}

TCPEgressGateway::TCPEgressGateway(int fd) : fd_(fd) {}

void TCPEgressGateway::setSocketFd(int fd) {
    fd_ = fd;
}

bool TCPEgressGateway::send(const uint8_t* buffer, size_t length) {
    if (fd_ < 0) {
        AsyncLogger::getInstance().log(LogLevel::ERROR, "TCPEgressGateway: Cannot send, invalid socket FD");
        return false;
    }

    size_t total_sent = 0;
    while (total_sent < length) {
        // MSG_NOSIGNAL prevents SIGPIPE if the remote end closed the connection
        ssize_t bytes_sent = ::send(fd_, buffer + total_sent, length - total_sent, MSG_NOSIGNAL);
        
        if (bytes_sent < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // In a true HFT system, we might spin or push to a queue here. 
                // For direct egress, if socket is full, we log and return false (dropped packet).
                AsyncLogger::getInstance().log(LogLevel::WARN, "TCPEgressGateway: TCP socket buffer full (EAGAIN)");
                return false;
            } else {
                AsyncLogger::getInstance().log(LogLevel::ERROR, "TCPEgressGateway: TCP send failed, errno=%d", errno);
                return false;
            }
        }
        
        total_sent += bytes_sent;
    }
    
    return true;
}

} // namespace HFT
