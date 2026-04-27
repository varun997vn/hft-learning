#pragma once

#include <atomic>
#include <thread>
#include <vector>
#include <string>
#include <cstdint>
#include <mutex>
#include <memory>
#include "SPSCQueue.hpp"

namespace HFT {

enum class LogLevel {
    INFO,
    WARN,
    ERROR,
    FATAL
};

constexpr size_t MAX_LOG_PAYLOAD = 240;
constexpr size_t QUEUE_CAPACITY = 1024;

// 256-byte aligned structure
struct alignas(64) LogMessage {
    uint64_t timestamp;
    LogLevel level;
    uint32_t thread_id;
    uint32_t length;
    char payload[MAX_LOG_PAYLOAD];
};

class AsyncLogger {
public:
    static AsyncLogger& getInstance() {
        static AsyncLogger instance;
        return instance;
    }

    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;
    AsyncLogger(AsyncLogger&&) = delete;
    AsyncLogger& operator=(AsyncLogger&&) = delete;

    // Initialize the logger with a specific file and initial mmap size (default 10MB)
    void init(const std::string& filepath, size_t mmap_size = 10 * 1024 * 1024);
    
    // Shut down the background thread and flush
    void shutdown();

    // Hot-path logging
    void log(LogLevel level, const char* format, ...);

    // Register a producer thread. Must be called by each thread that wants to log.
    void registerThread(uint32_t thread_id);

private:
    AsyncLogger();
    ~AsyncLogger();

    void backgroundThreadFunc();
    void writeToFile(const LogMessage& msg);
    void flush();
    void resizeMmap();

    std::string filepath_;
    int fd_;
    size_t mmap_size_;
    size_t current_offset_;
    char* mmap_ptr_;

    std::atomic<bool> running_;
    std::thread background_thread_;

    // Mutex for registering new threads (rare operation)
    std::mutex queues_mutex_;
    
    struct ThreadQueue {
        uint32_t thread_id;
        std::unique_ptr<SPSCQueue<LogMessage, QUEUE_CAPACITY>> queue;
    };
    std::vector<ThreadQueue> thread_queues_;
    
    // Thread-local pointer to the queue for O(1) access on the hot path
    static thread_local SPSCQueue<LogMessage, QUEUE_CAPACITY>* local_queue_;
    static thread_local uint32_t local_thread_id_;
};

#define LOG_INFO(...)  HFT::AsyncLogger::getInstance().log(HFT::LogLevel::INFO, __VA_ARGS__)
#define LOG_WARN(...)  HFT::AsyncLogger::getInstance().log(HFT::LogLevel::WARN, __VA_ARGS__)
#define LOG_ERROR(...) HFT::AsyncLogger::getInstance().log(HFT::LogLevel::ERROR, __VA_ARGS__)
#define LOG_FATAL(...) HFT::AsyncLogger::getInstance().log(HFT::LogLevel::FATAL, __VA_ARGS__)

} // namespace HFT
