#pragma once

#include "pch.hpp"

#include "exception.hpp"

//! @todo refactor and comment

namespace bklib {

struct pool_full_exception : virtual bklib::exception_base { };

enum class block_flags : uint8_t {
    free = 1,
    used = 2,
};

struct block_info {
    static uint32_t const END_INDEX = ~0U;

    uint8_t     key;
    block_flags flags;
    uint16_t    count;
    uint32_t    index;
};

template <typename T, size_t Size>
struct block_pool {
public:
    struct allocation {
        T*         pointer;
        block_info info;

        T* operator->() { return pointer; }
    };

    block_pool(uint8_t key = 0)
        : first_free_(0)
    {
        while (key == 0) {
            key = std::rand() % 0xFF;
        }

        uint32_t index = 0;
        
        std::generate_n(&state_[0], Size, [&] {
            block_info const result = {
                key,
                block_flags::free,
                1,
                ++index
            };

            return result;
        });
    }

    template <typename... Args>
    allocation alloc(Args&&... args) {
        auto const index = check_index_(first_free_);
        auto const next  = find_next_free_();

        auto& result_state = state_[index];
        auto& result_block = data_[index];

        new (&result_block) T(std::forward<Args>(args)...);

        result_state.flags = block_flags::used;
        result_state.index = index;

        first_free_ = next;

        allocation const result = {
            &result_block, result_state
        };
        
        return result;
    }

    void free(allocation a) {
    }
private:
    uint32_t check_index_(uint32_t i) const {
        if (i >= Size) {
            BOOST_THROW_EXCEPTION(pool_full_exception());
        }

        return i;
    }

    uint32_t find_next_free_() {
        auto const first = check_index_(first_free_);
        BK_ASSERT_MSG(state_[first].flags == block_flags::free, "invalid state!");
        
        auto const next = state_[first].index;
        return next < Size ? next : block_info::END_INDEX;
    }

    uint32_t                     first_free_;
    std::array<block_info, Size> state_;
    std::array<T, Size>          data_;
};


//! @todo change this
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
                return static_cast<unsigned>(index); //! @todo double check this
            }
        }

        BK_ASSERT_MSG(false, "Postcondition violated.");
        throw cache_exception("Unknown");
    }

    container_t_ c_;
};

} //namespace bklib
