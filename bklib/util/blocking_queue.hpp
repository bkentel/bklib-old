////////////////////////////////////////////////////////////////////////////////
//! @file
//! @author Brandon Kentel
//! @date   Jan 2013
//! @brief  Simple synchronized queue.
////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <mutex>
#include <condition_variable>
#include <queue>

namespace bklib {

//==============================================================================
//! Simple synchronized queue.
//==============================================================================
template <typename T>
class blocking_queue {
public:
    //--------------------------------------------------------------------------
    //! push an item and construct it in place using the T's move constructor.
    //--------------------------------------------------------------------------
    void emplace(T&& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.emplace(std::move(item));
        empty_condition_.notify_one();
    }

    void push(T const& item) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.emplace(std::move(item));
        empty_condition_.notify_one();
    }

    //--------------------------------------------------------------------------
    //! Pop an item from the queue; otherwise, block until a T is pushed.
    //--------------------------------------------------------------------------
    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);

        while (queue_.empty()) {
            empty_condition_.wait(lock);
        }

        auto result = std::move(queue_.front());
        queue_.pop();

        return result;
    }

    //--------------------------------------------------------------------------
    //! Unsynchronized check.
    //--------------------------------------------------------------------------
    bool empty() const {
        return queue_.empty();
    }
private:
    mutable std::mutex      mutex_;
    std::condition_variable empty_condition_;
    std::queue<T>           queue_;
};

} //namespace bklib
