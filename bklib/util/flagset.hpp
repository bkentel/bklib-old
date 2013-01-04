#pragma once

namespace bklib {

template <typename tag_t, size_t bit, typename storage_t = size_t>
struct flagset_flag {
    static_assert(bit < sizeof(storage_t)*8, "bit number out of range");
    
    static size_t const    bit   = bit;
    static storage_t const value = 1 << bit;
    typedef tag_t          type;
    typedef storage_t      storage;
};

template <typename tag_t, typename storage_t = size_t>
class flagset {
public:
    flagset(storage_t value = 0) : bits_(value) {
    }

    template <typename T>
    flagset(T flag,
        typename std::enable_if<
            std::is_same<typename T::type, tag_t>::value &&
            std::is_same<typename T::storage, storage_t>::value
        >::type* = nullptr
    ) : bits_(T::value) {
        check_types_<T>();
    }

    template <typename T>
    bool test() const {
        check_types_<T>();
        return bits_.test(T::bit);
    }

    template <typename T>
    void set(bool state = true) {
        check_types_<T>();
        bits_.set(T::bit, state);
    }

    template <typename T>
    void reset() {
        check_types_<T>();
        bits_.reset(T::bit)
    }

    template <typename T>
    static void check_types_() {
        static_assert(std::is_same<typename T::type, tag_t>::value, "mismatched types");
        static_assert(std::is_same<typename T::storage, storage_t>::value, "mismatched storage types");
    }
private:
    std::bitset<sizeof(storage_t)*8> bits_;
};

} //namespace bklib
