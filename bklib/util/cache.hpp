#pragma once

#include "pch.hpp"

namespace bklib {

struct cache_exception : public std::exception {
    explicit cache_exception(char const* what)
        : std::exception(what) { }
};

namespace detail {
    //opaque handle
    template <typename T>
    struct handle_t { };

    struct handle_info {
        template <typename T>
        explicit handle_info(handle_t<T> const* h)
            : handle(reinterpret_cast<uintptr_t>(h))
        {
            static_assert(sizeof(info_t) == sizeof(handle_info), "Wrong Size");
        }

        static unsigned const IS_FREE = 0xEE;
        static unsigned const IS_USED = 0xEF;

        struct info_t {
            unsigned index      : 16;
            unsigned count      : 8;
            unsigned reserved   : 8;
        };

        union {
            info_t    info;
            uintptr_t handle;
        };
    };

    struct handle_base_t {
        static uint16_t const RESERVED = 0xABCD;

        handle_base_t(uint32_t index, uint16_t count)
            : index(index), count(count), reserved(RESERVED)
        {
        }

        handle_base_t() : handle_base_t(0, 0) { }

        unsigned index    : 32; typedef uint32_t index_t;
        unsigned count    : 16; typedef uint16_t count_t;
        unsigned reserved : 16;
    };

    class cache_base_t {
    public:
        typedef handle_base_t handle_t;

        inline cache_base_t() : free_count_(0) { }

        inline unsigned free_slots() const {
            return free_count_;
        }
    protected:
        unsigned free_count_;
    };
} //namespace detail

template <typename T>
class cache_t : public detail::cache_base_t {
public:
    typedef std::shared_ptr<T> shared_t;
    typedef std::unique_ptr<T> unique_t;
private:
    struct record_t_ {
        record_t_(unique_t obj, handle_t::count_t count)
            : obj(std::move(obj)), count(count)
        {
        }

        record_t_(record_t_&& rhs)
            : record_t_(std::move(rhs.obj), rhs.count)
        {
        }

        unique_t          obj;
        handle_t::count_t count;
    };
public:
    template <typename... Args>
    handle_t construct(Args&&... args) {
        return add(
            std::make_unique<T>(std::forward<Args>(args)...)
        );
    }

    handle_t add(unique_t x) {
        //no free slots: construct at the back
        if (free_count_ == 0) {
            c_.emplace_back(std::move(x), 1);

            return handle_t(static_cast<uint32_t>(c_.size() - 1), 1);
        }

        //otherwise, find a free slot and reuse an existing entry
        auto const index = find_free_index_();
        auto const it    = c_.begin() + index;

        it->obj = std::move(x);
        free_count_--;

        return handle_t(index, it->count);
    }

    unique_t remove(handle_t handle) {
        auto& x = at_(handle);

        if (!is_free_(handle)) {
            free_count_++;
            x.count++; //increment the use count for this slot
        }

        return std::move(x.obj);
    }

    T& get(handle_t handle) const {
        auto& x = at_(handle);

        if (is_free_(handle)) {
            throw cache_exception("Object specified by handle is empty");
        }

        return *x.obj;
    }

    bool is_valid(handle_t handle) const {
        return is_valid_(handle) && !is_free_(handle);
    }

    unsigned size() const {
        return c_.size();
    }

    //TODO
    //Should be replaced by a proper iteratro implementation in the future
    template <typename F>
    void for_each(F&& f) {
        for (auto& i : c_) {
            if (i.obj && !f(*i.obj))
                break;
        }
    }

    template <typename F>
    void for_each(F&& f) const {
        for (auto const& i : c_) {
            if (i.obj) f(*i.obj);
        }
    }

    template <typename F>
    void for_each_reverse(F&& f) {
        std::for_each(c_.rbegin(), c_.rend(), [&](record_t_& i) {
            if (i.obj) f(*i.obj);
        });
    }

    template <typename F>
    void for_each_reverse(F&& f) const {
        std::for_each(c_.rbegin(), c_.rend(), [&](record_t_ const& i) {
            if (i.obj) f(*i.obj);
        });
    }
private:
    typedef std::vector<record_t_> container_t_;

    record_t_ const& at_(handle_t handle) const {
        if (!is_valid_(handle)) {
            throw cache_exception("Bad handle.");
        }

        return c_[handle.index];
    }

    record_t_& at_(handle_t handle) {
        if (!is_valid_(handle)) {
            throw cache_exception("Bad handle.");
        }

        return c_[handle.index];
    }


    bool is_free_(handle_t handle) const {
        return !c_[handle.index].obj;
    }

    bool is_valid_(handle_t handle) const {
        return (handle.reserved == handle_t::RESERVED) &&
               (handle.index < c_.size()) &&
               (c_[handle.index].count == handle.count);
    }

    unsigned find_free_index_() const {
        BK_ASSERT_MSG(free_count_ > 0, "Precondition violated.");

        auto const count       = c_.size();
        auto const start_index = std::rand() % count;

        for (unsigned i = 0; i < count; ++i) {
            auto const index = (start_index + i) % count;

            if (!c_[index].obj) {
                return index;
            }
        }

        BK_ASSERT_MSG(false, "Postcondition violated.");
        throw cache_exception("Unknown");
    }

    container_t_ c_;
};

} //namespace bklib
