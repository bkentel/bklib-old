#include <type_traits>
#include <iostream>

#include "windows.h"

template <typename tag_t, size_t bit, typename storage_t = size_t>
struct flag_set_flag {
    static_assert(bit < sizeof(storage_t)*8, "bit number out of range");
    static storage_t const value = 1 << bit;
    typedef tag_t type;
    typedef storage_t storage;
};

template <typename tag_t, typename storage_t = size_t>
struct flag_set {
    flag_set(storage_t value = 0) : value(value) { }

    template <typename T>
    bool test(
        typename std::enable_if<
            std::is_same<typename T::type, tag_t>::value &&
            std::is_same<typename T::storage, storage_t>::value
        >::type* = nullptr
    ) const {
        return (value & T::value) != 0;
    }

    template <typename T>
    void set(
        typename std::enable_if<
            std::is_same<typename T::type, tag_t>::value &&
            std::is_same<typename T::storage, storage_t>::value
        >::type* = nullptr
    ) {
        value |= T::value;
    }

    template <typename T>
    void clear(
        typename std::enable_if<
            std::is_same<typename T::type, tag_t>::value &&
            std::is_same<typename T::storage, storage_t>::value
        >::type* = nullptr
    ) {
        value &= ~T::value;
    }

    storage_t value;
};
    
struct my_flags {
    typedef flag_set_flag<my_flags, 0> first;
    typedef flag_set_flag<my_flags, 1> second;
    typedef flag_set_flag<my_flags, 2> third;
};




int main(int argc, char* argv[]) {

}
