#pragma once

#include <memory>
#include <new>

template <typename T>
struct aligned_deleter {
    std::align_val_t alignment;

    aligned_deleter() = delete;
    aligned_deleter(std::align_val_t val) : alignment { val } { }

    void operator()(T* ptr) {
        operator delete(ptr, alignment);
    }
};

template <typename T>
struct aligned_deleter<T[]> {
    std::align_val_t alignment;

    aligned_deleter() = delete;
    aligned_deleter(std::align_val_t val) : alignment { val } { }

    void operator()(T* ptr) {
        operator delete[](ptr, alignment);
    }
};

template <typename T>
using aligned_unique_ptr = std::unique_ptr<T, aligned_deleter<T>>;
