#pragma once

#include <vector>
#include <stdexcept>
#include <cstring> // For memset

template<typename T>
class ObjectPool {
public:
    explicit ObjectPool(size_t initial_size) {
        pool_.resize(initial_size);
        free_list_.reserve(initial_size);
        for (size_t i = 0; i < initial_size; ++i) {
            free_list_.push_back(&pool_[i]);
        }
    }

    T* NewOrder() {
        if (free_list_.empty()) {
            throw std::runtime_error("ObjectPool exhausted");
        }
        T* obj = free_list_.back();
        free_list_.pop_back();
        return obj;
    }

    void DeleteOrder(T* obj) {
        // Zero out memory to prevent stale data issues
        memset(obj, 0, sizeof(T));
        free_list_.push_back(obj);
    }

private:
    std::vector<T> pool_;
    std::vector<T*> free_list_;
};
