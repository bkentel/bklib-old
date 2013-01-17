#pragma once

namespace bklib {

//==============================================================================
//! Common base class for pool allocators.
//==============================================================================
class pool_allocator_base {
public:
    typedef uint64_t allocation;
    typedef uint32_t index_t;
protected:
    //--------------------------------------------------------------------------
    //! Allocation flags.
    //--------------------------------------------------------------------------
    enum class block_flags : uint8_t {
        free = 1,
        used = 2,
    };

    //--------------------------------------------------------------------------
    //! Control block info.
    //--------------------------------------------------------------------------
    struct block_info {
        //! For flags == free, this is the index of the next free block.
        //! For flags == used, this is the index of the allocation.
        index_t index;
        //! The number of times this block has been freed.
        uint16_t count;
        //! Unique key to help ensure aloocations from different pools are not
        //! mixed.
        uint8_t key;
        //! Flags describing the state of the aloocation block.
        block_flags flags;
    };

    static_assert(
        sizeof(allocation) == sizeof(block_info),
        "sizes don't match"
    );

    //! Sentinal index for allocations; indicates a full pool.
    static index_t const END_INDEX = ~0U;

    //! Constuct a pool capable of holding elements items.
    pool_allocator_base(uint32_t elements);

    //--------------------------------------------------------------------------
    //! Return the next free index, throw otherwise.
    //! @throw
    //--------------------------------------------------------------------------    
    index_t free_index_() const {
        if (free_ == END_INDEX) {
            BK_BREAK;
        }

        return free_;
    }

    //--------------------------------------------------------------------------
    //! If there are at least 2 free blocks, return the next next free index.
    //! If there is one free block, return END_INDEX.
    //! Otherwise, throw.
    //! @throw
    //--------------------------------------------------------------------------    
    index_t next_free_index_() const {
        return state_[free_index_()].index;
    }

    //! Set the block[where] to used, update the free list, and return the
    //! allocation value.
    allocation do_alloc_(index_t where, index_t next_free) {
        state_[where].index = where;
        state_[where].flags = block_flags::used;

        free_ = next_free;

        return reinterpret_cast<allocation&>(state_[where]);
    }

    //! Validate the allocation where and return its index.
    index_t get_allocation_index_(allocation where) const {
        auto const& i = reinterpret_cast<block_info&>(where);
        if (i.index >= elements_) {
            BK_BREAK;
        }

        auto const& j = state_[i.index];

        if (j.flags == block_flags::free) {
            BK_BREAK;
        } else if (j.count != i.count) {
            BK_BREAK;
        } else if (j.key != i.key) {
            BK_BREAK;
        }

        return i.index;
    }

    uint32_t                elements_; //<! number of allocation blocks.
    index_t                 free_;     //<! index of the first free block.
    std::vector<block_info> state_;    //<! state of each of the blocks.
};

//==============================================================================
//! A pool of memory blocks of size T.
//==============================================================================
template <typename T, typename Storage>
class pool_allocator : public pool_allocator_base {
public:
    typedef T const& const_reference;
    typedef Storage  storage;
    
    pool_allocator(uint32_t elements)
        : pool_allocator_base(elements)
        , data_(elements)
    {
    }

    allocation alloc() {
        auto const free_index = free_index_();
        auto const next_free_index = next_free_index_();

        return do_alloc_(free_index, next_free_index);
    }

    allocation alloc(const_reference value) {
        auto const free_index      = free_index_();
        auto const next_free_index = next_free_index_();

        data_.update(free_index, value);

        return do_alloc_(free_index, next_free_index);
    }

    template <typename U>
    void update(allocation where, U const& value, size_t offset = 0) {
        static_assert(sizeof(U) <= sizeof(T), "value type is too large.");
        BK_ASSERT(offset + sizeof(U) <= sizeof(T));
        data_.update(get_allocation_index_(where), value, offset);
    }

    index_t block_index(allocation where) const {
        return get_allocation_index_(where);
    }

    storage& buffer() {
        return data_;
    }

    storage const& buffer() const {
        return data_;
    }
private:
    storage data_;
};


} // namespace bklib
