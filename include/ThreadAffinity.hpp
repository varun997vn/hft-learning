#pragma once

#include <thread>

namespace ThreadPinning {

    /**
     * @brief Pins a given thread to a specific CPU core.
     * @param handle The native handle of the thread (e.g., from std::thread::native_handle()).
     * @param core_id The logical CPU core ID to pin the thread to.
     * @return true if successful, false otherwise.
     */
    bool pin_thread_to_core(std::thread::native_handle_type handle, int core_id);

    /**
     * @brief Pins the currently executing thread to a specific CPU core.
     * @param core_id The logical CPU core ID to pin the thread to.
     * @return true if successful, false otherwise.
     */
    bool pin_current_thread_to_core(int core_id);

} // namespace ThreadPinning
