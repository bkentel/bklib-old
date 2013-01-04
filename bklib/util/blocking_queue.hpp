#pragma once

namespace bklib {

template <typename T>
class blocking_queue {
public:
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

    T pop() {
        std::unique_lock<std::mutex> lock(mutex_);

        while (queue_.empty()) {
            empty_condition_.wait(lock);
        }

        auto result = std::move(queue_.front());
        queue_.pop();

        return result;
    }

    bool empty() const {
        return queue_.empty();
    }
private:
    std::mutex              mutex_;
    std::condition_variable empty_condition_;
    std::queue<T>           queue_;
};


} //namespace bklib
