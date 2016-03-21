#pragma once
#include <stdint.h>
#include <assert.h>
#include <atomic>

#if defined(_MSC_VER)
    #include <intrin.h>
    #define AtomicCompareAndSwap(addr, desired, expected) _InterlockedCompareExchange64(addr, desired, expected)
#else
    #define AtomicCompareAndSwap(addr, desired, expected) __sync_val_compare_and_swap(addr, expected, desired)
#endif

template<uint32_t kMaxCount = 1024>
class TaskQueue {
public:
    /// @brief Returns a rough estimate of how many items are in the queue. Note that
    ///     this is only an estimate because other threads can change the size during
    ///     this call.
    int64_t size()const
    {
        return this->_bottom - this->_top;
    }

    /// @brief Pushes a new item onto the bottom of the queue
    /// @return 0 on success, 1 on failure (queue is full)
    int push(struct Task* value)
    {
        if (this->size() == kMaxCount) {
            return 1;
        }

        int64_t const bottom = this->_bottom;
        this->_data[bottom & kQueueMask] = value;

        std::atomic_signal_fence(std::memory_order_seq_cst);

        this->_bottom = bottom + 1;

        return 0;
    }

    Task* pop()
    {
        int64_t bottom = this->_bottom - 1;
        this->_bottom = bottom;
        std::atomic_thread_fence(std::memory_order_seq_cst);
        int64_t top = this->_top;

        if (top <= bottom) {
            struct Task* value = this->_data[bottom & kQueueMask];
            if (top != bottom) {
                return value;
            }
            if (AtomicCompareAndSwap(&this->_top, top + 1, top) != top) {
                value = NULL;
            }
            this->_bottom = top + 1;
            return value;
        } else {
            this->_bottom = top;
            return NULL;
        }
    }

    Task* steal()
    {
        int64_t top = this->_top;
        std::atomic_signal_fence(std::memory_order_seq_cst);
        int64_t bottom = this->_bottom;

        if (top < bottom) {
            struct Task* value = this->_data[top & kQueueMask];
            if (AtomicCompareAndSwap(&this->_top, top + 1, top) != top) {
                return NULL;
            }
            return value;
        } else {
            return NULL;
        }
    }

private:
    enum {
        kQueueMask = kMaxCount - 1,
    };

    struct Task*        _data[kMaxCount] = {nullptr};
    volatile int64_t    _top = 0;
    volatile int64_t    _bottom = 0;
};
