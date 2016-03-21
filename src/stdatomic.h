/// @brief C11 thread header for MSVC. Wraps C++11 methods with C11 signatures
#pragma once
#include <stdint.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum memory_order {
    memory_order_relaxed,
    memory_order_consume,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst
} memory_order;

void std_atomic_signal_fence( memory_order order );
void std_atomic_thread_fence( memory_order order );

#ifdef __cplusplus
} // extern "C" {
#endif /* __cplusplus */
