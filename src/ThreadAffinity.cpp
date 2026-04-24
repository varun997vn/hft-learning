#include "../include/ThreadAffinity.hpp"
#include <pthread.h>
#include <sched.h>
#include <iostream>

namespace ThreadPinning {

    bool pin_thread_to_core(std::thread::native_handle_type handle, int core_id) {
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(core_id, &cpuset);

        // pthread_setaffinity_np returns 0 on success
        int rc = pthread_setaffinity_np(handle, sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
            return false;
        }

        return true;
    }

    bool pin_current_thread_to_core(int core_id) {
        return pin_thread_to_core(pthread_self(), core_id);
    }

} // namespace ThreadPinning
