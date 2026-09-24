#pragma once

// Single-producer / single-consumer lock-free queue of trivially copyable items with a
// fixed capacity (power of two). Used to hand data from the message thread to the audio
// thread without locks or allocation.

#include <array>
#include <atomic>
#include <cstddef>
#include <type_traits>

namespace arc
{

template <typename T, size_t Capacity>
class SpscQueue
{
    static_assert ((Capacity & (Capacity - 1)) == 0, "Capacity must be a power of two");
    static_assert (std::is_trivially_copyable_v<T>, "T must be trivially copyable");

public:
    /** Producer side. Returns false when full (the item is dropped). */
    bool push (const T& item) noexcept
    {
        const size_t w = writeIndex.load (std::memory_order_relaxed);
        const size_t r = readIndex.load (std::memory_order_acquire);
        if (w - r >= Capacity)
            return false;
        items[w & (Capacity - 1)] = item;
        writeIndex.store (w + 1, std::memory_order_release);
        return true;
    }

    /** Consumer side. */
    bool pop (T& out) noexcept
    {
        const size_t r = readIndex.load (std::memory_order_relaxed);
        const size_t w = writeIndex.load (std::memory_order_acquire);
        if (r == w)
            return false;
        out = items[r & (Capacity - 1)];
        readIndex.store (r + 1, std::memory_order_release);
        return true;
    }

private:
    std::array<T, Capacity> items {};
    std::atomic<size_t> writeIndex { 0 }, readIndex { 0 };
};

} // namespace arc
