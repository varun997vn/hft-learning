#include "../include/TSCUtils.hpp"
#include <chrono>
#include <thread>
#include <iostream>

namespace TSCUtils {

    double estimate_tsc_frequency_hz() {
#if defined(__x86_64__) || defined(__i386__)
        std::cout << "Estimating TSC frequency..." << std::endl;
        
        auto start_time = std::chrono::high_resolution_clock::now();
        uint64_t start_tsc = read_tsc();
        
        // Sleep for 100 milliseconds
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        uint64_t end_tsc = read_tsc();
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        uint64_t tsc_delta = end_tsc - start_tsc;
        
        double frequency_hz = (static_cast<double>(tsc_delta) / duration_ns) * 1e9;
        std::cout << "Estimated TSC frequency: " << (frequency_hz / 1e9) << " GHz" << std::endl;
        
        return frequency_hz;
#else
        return 0.0;
#endif
    }

} // namespace TSCUtils
