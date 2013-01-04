#include "pch.hpp"

#include <type_traits>
#include <memory>

#include "exception.hpp"

namespace bklib {
//==============================================================================
//! Endian type.
//==============================================================================
enum class endian_type : uint32_t {
    little = 0,
    big    = 1,
};

//==============================================================================
//! Convert the integral type T to endian_type.
//==============================================================================
template <typename T>
T endian(T in, endian_type endian) {
    static_assert(std::is_integral<T>::value, "not an integer type");

    union {
        uint64_t quad;
        uint32_t is_little;
    } test;
    
    test.quad = 1;
    if (test.is_little ^ static_cast<uint32_t>(endian)) return in;
    
    T result = 0;

    // decent compilers will unroll this (gcc)
    // or even convert straight into single bswap (clang)
    for (size_t i = 0; i < sizeof(T); ++i) {
        result <<= 8;
        result |= in & 0xFF;
        in >>= 8;
    }

    return result;
};

//******************************************************************************
#pragma pack(push, tga, 1) //Enforce tight packing
//******************************************************************************
//==============================================================================
//! Targa image files.
//==============================================================================
namespace tga {

struct targa_exception : virtual exception_base { };

typedef uint8_t byte;

//==============================================================================
//! Color map type.
//==============================================================================
enum class color_map_type : byte {
    absent  = 0,
    present = 1,
};

//==============================================================================
//! Type of image data.
//==============================================================================
enum class image_type : byte {
    none             = 0,
    color_mapped     = 1,
    true_color       = 2,
    black_white      = 3,
    rle_color_mapped = 9,
    rle_true_color   = 10,
    rle_black_white  = 11,
};
   
struct color_map_spec {
    uint16_t first_entry_index;
    uint16_t length;
    byte     entry_size;
};
static_assert(sizeof(color_map_spec) == 5, "bad size"); // sanity check.

struct image_descriptor {
    byte alpha    : 4;
    byte right    : 1;
    byte top      : 1;
    byte reserved : 2;
};
static_assert(sizeof(image_descriptor) == 1, "bad size"); // sanity check.

struct image_spec {
    uint16_t         x_origin;
    uint16_t         y_origin;
    uint16_t         width;
    uint16_t         height;
    byte             depth;
    image_descriptor descriptor;
};
static_assert(sizeof(image_spec) == 10, "bad size"); // sanity check.

//==============================================================================
//! Header for all Targa files.
//==============================================================================
struct header {
    byte           id_length;
    color_map_type color_map_type;
    image_type     image_type;
    color_map_spec color_map_spec;
    image_spec     image_spec;
};
static_assert(sizeof(header) == 18, "bad size");  // sanity check.

//==============================================================================
//! Footer for newer Targa file format.
//==============================================================================
struct footer {
    uint32_t ext_area_offset;
    uint32_t dev_dir_offset;
    char     signature[16];
    char     dot_terminator;
    char     null_terminator;

    bool is_valid() const;
};
//******************************************************************************
#pragma pack(pop, tga) //Enforce tight packing
//******************************************************************************

//==============================================================================
//! Targa image file.
//==============================================================================
class image {
public:
    explicit image(utf8string filename);

    void const* cbegin() const {
        return data_.get();
    }

    void const* cend() const {
        return data_.get() + data_size_;
    }

    size_t size() const {
        return data_size_;
    }

    unsigned width() const {
        return header_.image_spec.width;
    }

    unsigned height() const {
        return header_.image_spec.height;
    }
private:
    void load_ver_2(std::ifstream& in);
    void load_ver_1(std::ifstream& in);

    header                  header_;
    std::unique_ptr<char[]> data_;
    size_t                  data_size_;
};

} // namespace tga
} // namespace bklib
