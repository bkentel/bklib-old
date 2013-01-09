#include "pch.hpp"

#include "pool_allocator.hpp"

bklib::pool_allocator_base::pool_allocator_base(uint32_t elements)
    : elements_(elements)
    , free_(0)
{
    index_t index = 0;
    uint8_t  key   = 1;

    // initialize the free list
    std::generate_n(std::back_inserter(state_), elements, [&] {
        block_info const result = {
            ++index, 1, key, block_flags::free
        };

        return result;
    });

    state_.back().index = END_INDEX;
}
