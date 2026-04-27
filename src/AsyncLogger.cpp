#include "AsyncLogger.hpp"
#include "TSCUtils.hpp"
#include <iostream>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

namespace HFT {

thread_local SPSCQueue<LogMessage, QUEUE_CAPACITY>* AsyncLogger::local_queue_ = nullptr;
thread_local uint32_t AsyncLogger::local_thread_id_ = 0;

AsyncLogger::AsyncLogger() 
    : fd_(-1), mmap_size_(0), current_offset_(0), mmap_ptr_(static_cast<char*>(MAP_FAILED)), running_(false) {
}

AsyncLogger::~AsyncLogger() {
    shutdown();
}

void AsyncLogger::init(const std::string& filepath, size_t mmap_size) {
    if (running_.load()) return;
    
    filepath_ = filepath;
    mmap_size_ = mmap_size;
    
    // Open file
    fd_ = ::open(filepath_.c_str(), O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd_ == -1) {
        std::cerr << "AsyncLogger: Failed to open file " << filepath_ << "\n";
        return;
    }
    
    // Truncate to desired size
    if (::ftruncate(fd_, mmap_size_) == -1) {
        std::cerr << "AsyncLogger: Failed to ftruncate\n";
        ::close(fd_);
        fd_ = -1;
        return;
    }
    
    // Map memory
    mmap_ptr_ = static_cast<char*>(::mmap(nullptr, mmap_size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd_, 0));
    if (mmap_ptr_ == MAP_FAILED) {
        std::cerr << "AsyncLogger: Failed to mmap\n";
        ::close(fd_);
        fd_ = -1;
        return;
    }
    
    running_.store(true);
    background_thread_ = std::thread(&AsyncLogger::backgroundThreadFunc, this);
}

void AsyncLogger::shutdown() {
    if (!running_.exchange(false)) return; // Already shut down
    
    if (background_thread_.joinable()) {
        background_thread_.join();
    }
    
    if (mmap_ptr_ != MAP_FAILED) {
        // Unmap and close file, truncate to actual used size
        ::munmap(mmap_ptr_, mmap_size_);
        ::ftruncate(fd_, current_offset_);
        ::close(fd_);
        mmap_ptr_ = static_cast<char*>(MAP_FAILED);
        fd_ = -1;
    }
}

void AsyncLogger::registerThread(uint32_t thread_id) {
    std::lock_guard<std::mutex> lock(queues_mutex_);
    
    // Check if already registered
    for (const auto& tq : thread_queues_) {
        if (tq.thread_id == thread_id) {
            local_queue_ = tq.queue.get();
            local_thread_id_ = thread_id;
            return;
        }
    }
    
    // Create new queue
    auto queue = std::make_unique<SPSCQueue<LogMessage, QUEUE_CAPACITY>>();
    local_queue_ = queue.get();
    local_thread_id_ = thread_id;
    
    thread_queues_.push_back({thread_id, std::move(queue)});
}

void AsyncLogger::log(LogLevel level, const char* format, ...) {
    if (!local_queue_) return; // Thread not registered
    
    LogMessage msg;
    msg.timestamp = TSCUtils::read_tsc(); 
    msg.level = level;
    msg.thread_id = local_thread_id_;
    
    va_list args;
    va_start(args, format);
    int len = vsnprintf(msg.payload, MAX_LOG_PAYLOAD, format, args);
    va_end(args);
    
    if (len < 0) {
        msg.length = 0;
    } else if (len >= static_cast<int>(MAX_LOG_PAYLOAD)) {
        msg.length = MAX_LOG_PAYLOAD - 1;
    } else {
        msg.length = len;
    }
    
    // Push to lock-free queue. If full, spin-wait (or could be configured to drop)
    while (!local_queue_->push(msg)) {
        // Yield to prevent deadlocking if pinned to same core as background thread
        std::this_thread::yield(); 
    }
}

void AsyncLogger::resizeMmap() {
    size_t new_size = mmap_size_ * 2;
    if (::ftruncate(fd_, new_size) == -1) {
        std::cerr << "AsyncLogger: Failed to extend file\n";
        return; 
    }
    
    char* new_ptr = static_cast<char*>(::mremap(mmap_ptr_, mmap_size_, new_size, MREMAP_MAYMOVE));
    if (new_ptr == MAP_FAILED) {
        std::cerr << "AsyncLogger: Failed to mremap\n";
        return;
    }
    
    mmap_ptr_ = new_ptr;
    mmap_size_ = new_size;
}

const char* levelToString(LogLevel level) {
    switch (level) {
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARN: return "WARN";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

void AsyncLogger::writeToFile(const LogMessage& msg) {
    char buffer[512];
    int len = snprintf(buffer, sizeof(buffer), "[%lu] [T:%u] [%s] %s\n", 
                       msg.timestamp, msg.thread_id, levelToString(msg.level), msg.payload);
                       
    if (len < 0) return;
    
    if (current_offset_ + len > mmap_size_) {
        resizeMmap();
    }
    
    if (mmap_ptr_ != MAP_FAILED && current_offset_ + len <= mmap_size_) {
        std::memcpy(mmap_ptr_ + current_offset_, buffer, len);
        current_offset_ += len;
    }
}

void AsyncLogger::backgroundThreadFunc() {
    LogMessage msg;
    std::vector<SPSCQueue<LogMessage, QUEUE_CAPACITY>*> active_queues;
    
    while (running_.load(std::memory_order_relaxed) || !active_queues.empty()) {
        bool idle = true;
        
        // Safely update the list of active queues if new ones were registered
        {
            std::lock_guard<std::mutex> lock(queues_mutex_);
            if (active_queues.size() != thread_queues_.size()) {
                active_queues.clear();
                for (const auto& tq : thread_queues_) {
                    active_queues.push_back(tq.queue.get());
                }
            }
        }
        
        // Poll all queues
        for (auto* queue : active_queues) {
            while (queue->pop(msg)) {
                writeToFile(msg);
                idle = false;
            }
        }
        
        // Small yield to prevent 100% CPU usage if idle, 
        // in a pure HFT scenario we might just spin or use a wait-free yielding strategy
        if (idle && running_.load(std::memory_order_relaxed)) {
            std::this_thread::yield(); 
        }
        
        // Break out if we are shutting down and queues are empty
        if (!running_.load(std::memory_order_relaxed) && idle) {
            break;
        }
    }
}

} // namespace HFT
