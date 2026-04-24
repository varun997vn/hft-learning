#pragma once

#include <cstdint>
#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#endif

namespace TSCUtils {

    /**
     * @brief Reads the Time Stamp Counter (TSC).
     * Uses __rdtscp to ensure all previous instructions have executed,
     * preventing the CPU from reordering the read ahead of time.
     */
    inline uint64_t read_tsc() {
#if defined(__x86_64__) || defined(__i386__)
        unsigned int aux;
        return __rdtscp(&aux);
#else
        // Fallback for non-x86 architectures
        return 0; 
#endif
    }

    /**
     * @brief Estimates the TSC frequency in Hz by sleeping for a known duration.
     * @return The estimated frequency of the TSC in Hz.
     */
    double estimate_tsc_frequency_hz();

    /**
     * @brief Converts a TSC cycle delta to nanoseconds.
     * @param cycles The number of cycles delta.
     * @param frequency_hz The measured frequency of the TSC.
     * @return The equivalent duration in nanoseconds.
     */
    inline double cycles_to_nanoseconds(uint64_t cycles, double frequency_hz) {
        if (frequency_hz == 0.0) return 0.0;
        return (static_cast<double>(cycles) / frequency_hz) * 1e9;
    }

} // namespace TSCUtils
