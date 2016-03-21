/// @brief C11 thread header for MSVC. Wraps C++11 methods with C11 signatures
#include "stdatomic.h"
#include <atomic>
#include <memory>
#include <thread>

void std_atomic_signal_fence( memory_order order )
{
    std::atomic_signal_fence((std::memory_order)order);
}

void std_atomic_thread_fence( memory_order order )
{
    std::atomic_thread_fence((std::memory_order)order);
}

int std_thrd_create(thrd_t* thr, thrd_start_t func, void* arg)
{
    *thr = new std::thread(func, arg);
    return thrd_success;
}

int std_thrd_join(thrd_t thr, int* res)
{
    std::thread* cpp_thread = (std::thread*)thr;
    cpp_thread->join();
    delete cpp_thread;
    *res = 0;
    return thrd_success;
}
