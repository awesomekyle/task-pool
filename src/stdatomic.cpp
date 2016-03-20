/// @brief C11 thread header for MSVC. Wraps C++11 methods with C11 signatures
#include "stdatomic.h"
#include <atomic>

void std_atomic_signal_fence( memory_order order )
{
    std::atomic_signal_fence((std::memory_order)order);
}

void std_atomic_thread_fence( memory_order order )
{
    std::atomic_thread_fence((std::memory_order)order);
}
