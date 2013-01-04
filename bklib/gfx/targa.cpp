#include "pch.hpp"
#include "targa.hpp"

#include "util/macros.hpp"

namespace tga = ::bklib::tga;

tga::image::image(bklib::utf8string filename)
try {
    // open as read only binary and seek to end
    std::ifstream in(filename, std::ios::binary | std::ios::in | std::ios::ate);
    in.exceptions(std::ios_base::badbit | std::ios_base::failbit);
        
    //seek back from the end and read the footer
    tga::footer footer;
    in.seekg(0 - sizeof(footer), std::ios::cur);
    in.read(reinterpret_cast<char*>(&footer), sizeof(footer));

    if (footer.is_valid()) {
        load_ver_2(in);
    } else {
        load_ver_1(in);
    }
} catch (bklib::exception_base& e) {
    e << boost::errinfo_file_name(filename);
    throw;
}

void tga::image::load_ver_2(std::ifstream& in) {
    load_ver_1(in);   
}

void tga::image::load_ver_1(std::ifstream& in) {
    in.seekg(0, std::ios::beg);
    in.read(reinterpret_cast<char*>(&header_), sizeof(header_));

    if (header_.color_map_type != tga::color_map_type::absent) {
        BOOST_THROW_EXCEPTION(targa_exception()
            << bklib::error_message("color mapped images not supported.")
        );
    } else if (header_.image_type != tga::image_type::true_color) {
        BOOST_THROW_EXCEPTION(targa_exception()
            << bklib::error_message("non true color images not supported.")
        );
    }

    auto const descriptor = header_.image_spec.descriptor;
    if (descriptor.top != 0 && descriptor.right != 0) {
        BOOST_THROW_EXCEPTION(targa_exception()
            << bklib::error_message("rotated images not supported.")
        );
    }

    auto const depth = header_.image_spec.depth;
    auto const alpha = descriptor.alpha;

    switch (depth) {
        case 16 : if (alpha == 1) break;
        case 24 : if (alpha == 0) break;
        case 32 : if (alpha == 8) break;
        default :
            BOOST_THROW_EXCEPTION(targa_exception()
                << bklib::error_message("invalid alpha value.")
            );
        ;
    }

    if (header_.color_map_spec.first_entry_index != 0 ||
        header_.color_map_spec.entry_size        != 0 ||
        header_.color_map_spec.length            != 0
    ) {
        BOOST_THROW_EXCEPTION(targa_exception()
            << bklib::error_message("color mapped images not supported.")
        );
    }

    // skip past the id
    in.seekg(header_.id_length, std::ios::cur);

    auto const w = header_.image_spec.width;
    auto const h = header_.image_spec.height;

    auto const ww = endian(w, endian_type::little);
    std::cout << ww;

    data_size_ = w * h * (depth / 8);
    auto const scanline_size = w * (depth / 8);

    data_.reset(new char[data_size_]);
    in.read(data_.get(), data_size_);

    // flip the image vertically
    std::unique_ptr<char[]> temp(new char[scanline_size]);
    for (size_t i = 0; i < h / 2; ++i) {
        auto const a = data_.get() + i*scanline_size;
        auto const b = data_.get() + data_size_ - (i+1)*scanline_size;

        std::memcpy(temp.get(), a, scanline_size);
        std::memcpy(a, b, scanline_size);
        std::memcpy(b, temp.get(), scanline_size);
    }
}

bool tga::footer::is_valid() const {
    static char const FOOTER_SIGNATURE[] = "TRUEVISION-XFILE";

    if (std::strncmp(
        FOOTER_SIGNATURE,
        signature,
        BK_ARRAY_ELEMENT_COUNT(signature)) == 0
    ) {
        return false;
    } else if (dot_terminator  != '.') {
        return false;
    } else if (null_terminator != 0) {
        return false;
    }

    return true;
}
