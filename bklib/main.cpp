#include "pch.hpp"

#include "window/window.hpp"
#include "input/input.hpp"
//#include "gfx/renderer/renderer2d/renderer2d.hpp"
//#include "gui/gui.hpp"

//#include "gfx/targa.hpp"

#include "util/blocking_queue.hpp"

#include "gui/gui2.hpp"

#include "gfx/text.hpp"

template <size_t N, typename tag_t, typename storage_t = unsigned>
struct color_component {
    static size_t const bits = N;
    typedef tag_t type;
    typedef storage_t storage;
};

namespace color {
    struct tag_none;
    struct tag_r;
    struct tag_g;
    struct tag_b;
    struct tag_a;

    template <typename T>
    struct none : public color_component<0, tag_none, typename T::storage_t> {
    };

    typedef color_component<8, tag_r> r8;
    typedef color_component<8, tag_g> g8;
    typedef color_component<8, tag_b> b8;
    typedef color_component<8, tag_a> a8;
}

template <
    typename C1,
    typename C2 = color::none<C1>,
    typename C3 = color::none<C2>,
    typename C4 = color::none<C3>
>
struct color_format {
    static_assert(
        std::is_same<typename C1::storage_t, typename C2::storage_t>::value &&
        std::is_same<typename C2::storage_t, typename C3::storage_t>::value &&
        std::is_same<typename C3::storage_t, typename C4::storage_t>::value
        , "mismatched storage types"
    );

    static size_t const bits = C1::bits + C2::bits + C3::bits + C4::bits;
};

typedef color_format<color::b8, color::g8, color::r8>            tga_24;
typedef color_format<color::b8, color::g8, color::r8, color::a8> tga_32;

typedef color_format<color::b8, color::g8, color::r8, color::a8> bgra8;

template <typename src, typename dest>
void convert(
    void const* in_begin,  void const* in_end,
    void*       out_begin, void*       out_end
);

template <>
void convert<tga_24, bgra8> (
    void const* in_begin,  void const* in_end,
    void*       out_begin, void*       out_end
) {
    auto const in_size  = (ptrdiff_t)in_end  - (ptrdiff_t)in_begin;
    auto const out_size = (ptrdiff_t)out_end - (ptrdiff_t)out_begin;

    BK_ASSERT_MSG(in_size  >= 0, "bad pointers");
    BK_ASSERT_MSG(out_size >= 0, "bad pointers");
    BK_ASSERT_MSG(in_size  % 3 == 0, "wrong size");
    BK_ASSERT_MSG(out_size % 4 == 0, "wrong size");
    BK_ASSERT_MSG(out_size - in_size == in_size / 3, "wrong size");

    auto in  = reinterpret_cast<char const*>(in_begin);
    auto out = reinterpret_cast<char*>(out_begin);

    while (in != reinterpret_cast<char const*>(in_end)) {
        *reinterpret_cast<uint32_t*>(out) = *reinterpret_cast<uint32_t const*>(in);
        out[3] = 0;
        
        in  += 3;
        out += 4;
    }
}


//struct tile_set {
//    tile_set(unsigned tile_w, unsigned tile_h, unsigned texture_w, unsigned texture_h)
//        : tile_width_(tile_w), tile_height_(tile_h)
//        , texture_width_(texture_w), texture_height_(texture_h)
//    {
//        BK_ASSERT_MSG(texture_width_  % tile_width_  == 0, "mismatched sizes");
//        BK_ASSERT_MSG(texture_height_ % tile_height_ == 0, "mismatched sizes");
//    }
//
//    bklib::gfx2d::rect get(unsigned index) const {
//        auto const y = index * tile_width_ / texture_width_;
//        auto const x = ((index * tile_width_) - (y * texture_width_)) / tile_width_;
//
//        return bklib::gfx2d::rect(
//            bklib::math::point<float, 2>(
//                static_cast<float>(x * tile_width_),
//                static_cast<float>(y * tile_height_)
//            ),
//            static_cast<float>(tile_width_),
//            static_cast<float>(tile_height_)
//        );
//    }
//
//    unsigned tile_width_;
//    unsigned tile_height_;
//    unsigned texture_width_;
//    unsigned texture_height_;
//};

////////////////////////////////////////////////////////////////////////////////
#include "gfx/renderer2d.hpp"
#include "gfx/gfx.hpp"

int main(int argc, char const* argv[]) {
    using namespace bklib;

    window::promise_t is_created;   
    window win(is_created);
    auto ime_manager = is_created.get_future().get();

    win.activate_gl();
    gfx::renderer2d gui_renderer;

    gui::root root(gui_renderer);

    //--------------------------------------------------------------------------
    // event handlers
    //--------------------------------------------------------------------------
    win.listen<window::event_on_mouse_move_to>([&](signed x, signed y) {
        root.on_mouse_move_to(x, y);
    });
    //--------------------------------------------------------------------------
    win.listen<window::event_on_mouse_down>([&](unsigned button) {
        root.on_mouse_down(button);
    });
    //--------------------------------------------------------------------------
    win.listen<window::event_on_mouse_up>([&](unsigned button) {
        root.on_mouse_up(button);
    });
    //--------------------------------------------------------------------------
    unsigned window_w = 0;
    unsigned window_h = 0;
    win.listen<window::event_on_size>([&](unsigned w, unsigned h) {
        window_w = w;
        window_h = h;
    });
    //--------------------------------------------------------------------------
    bool quit_flag = false;
    win.listen<window::event_on_close>([&] {
        win.deactivate_gl();
        win.close();
        quit_flag = true;
    });
    //--------------------------------------------------------------------------
    
    auto win0 = root.make_widget<gui::window>(
        gui::rect(gui::point(10, 10), 640, 480)
    );
    auto win1 = root.make_widget<gui::window>(
        gui::rect(gui::point(100, 100), 640, 480)
    );
    auto win2 = root.make_widget<gui::window>(
        gui::rect(gui::point(200, 200), 640, 480)
    );

    root.add_widget(std::move(win0));
    root.add_widget(std::move(win1));
    root.add_widget(std::move(win2));

    gfx::font_renderer text_renderer;

    auto const render_function = [&] {
        gui_renderer.set_viewport(window_w, window_h);

        ::glClearColor(0.5f, 0.0f, 0.0f, 1.0f);
        ::glClear(GL_COLOR_BUFFER_BIT);
        
        root.draw();

        win.swap_buffers();
    };

    win.show();

    while (!quit_flag) {
        win.do_pending_events();
        ime_manager->do_pending_events();
        Sleep(1);

        if (quit_flag) break;
        
        render_function();
    }

    win.wait();
}

//int main(int argc, char const* argv[])
//try {
//    using namespace bklib;
//
//    ////////////////////////////////////////////////////////////////////////////////
//
//    //create the system window and wait until a future signaling its completion
//    //is ready
//    window::promise_t is_created;
//    auto is_created_future = is_created.get_future();
//    window win(is_created);
//
//    is_created_future.wait();
//    auto ime_manager = is_created_future.get();
//
//    gui::root       gui_root(ime_manager);
//    gfx2d::renderer renderer(win);
//
//    ////////
//    tga::image image("tiles.tga");
//
//    std::vector<char> converted((image.size() / 3)*4, 0);
//
//    convert<tga_24, bgra8>(
//        image.cbegin(), image.cend(),
//        converted.data(), converted.data() + converted.size()
//    );
//
//    renderer.create_texture(image.width(), image.height(), converted.data());
//    
//    auto image_rect_src  = bklib::gfx2d::rect(16, 16, 32, 32);
//    
//    ////////
//
//    //set up event handlers
//    auto on_paint = [&] {
//        renderer.draw_begin();
//        
//        renderer.clear(bklib::gfx2d::color(0.5f, 0.5f, 0.0f));
//        
//        for (int x = 0; x < 16; ++x) {
//            for (int y = 0; y < 16; ++y) {
//                auto image_rect_dest = bklib::gfx2d::rect(x*16, y*16, x*16+16, y*16+16);
//                renderer.draw_texture(image_rect_src, image_rect_dest);
//            }
//        }
//        
//        gui_root.draw(renderer);
//
//        renderer.draw_end();
//    };
//
//    // on_key_down
//    win.listen<window::event_on_key_down>([&](window::key_code_t key) {
//        gui_root.on_key_down(key);
//    });
//    // on_key_up
//    win.listen<window::event_on_key_up>([&](window::key_code_t key) {
//        gui_root.on_key_up(key);
//    });
//    // on_input_char
//    win.listen<window::event_on_input_char>([&](bklib::utf32codepoint cp) {
//        gui_root.on_input_char(cp);
//    });
//    // on_mouse_move_to
//    win.listen<window::event_on_mouse_move_to>([&](int x, int y) {
//        gui_root.on_mouse_move_to(x, y);
//    });
//    // on_mouse_move
//    win.listen<window::event_on_mouse_move>([&](int dx, int dy) {
//    });
//    // on_mouse_down
//    win.listen<window::event_on_mouse_down>([&](int button) {
//        gui_root.on_mouse_down(button);
//    });
//    // on_mouse_up
//    win.listen<window::event_on_mouse_up>([&](int button) {
//        gui_root.on_mouse_up(button);
//    });
//    // on_mouse_scroll
//    win.listen<window::event_on_mouse_scroll>([&](int scroll) {
//    });
//
//    bool quit_flag = false;
//    win.listen<window::event_on_close>([&]() {
//        win.close();
//        quit_flag = true;
//    });
//
//    win.listen<window::event_on_paint>([&] {
//        //on_paint();
//    });
//
//    win.listen<window::event_on_size>([&](unsigned w, unsigned h) {
//        renderer.resize(w, h);
//    });
//
//    ////////////////////
//    {
//        using namespace gui;
//
//        auto w = std::make_unique<gui::window>(rect(point(10.0f, 10.0f), 320.0f, 240.0f));
//
//        auto input1 = std::make_unique<gui::input>(rect(point(10.0f, 10.0f), 200.0f, 24.0f));
//        auto input2 = std::make_unique<gui::input>(rect(point(10.0f, 44.0f), 200.0f, 24.0f));
//        auto input3 = std::make_unique<gui::input>(rect(point(10.0f, 78.0f), 200.0f, 24.0f));
//
//        w->add_child(std::move(input1));
//        w->add_child(std::move(input2));
//        w->add_child(std::move(input3));
//
//        gui_root.add_child(std::move(w));
//    }
//
//    gui_root.listen<gui::root::event_on_update>(on_paint);
//    ////////////////////
//
//    win.show();
//
//    while (!quit_flag) {
//        win.do_pending_events();
//        ime_manager->do_pending_events();
//        Sleep(1);
//        on_paint();
//    }
//
//    win.wait();
//
//    return EXIT_SUCCESS;
//} catch (...) {
//    return EXIT_FAILURE;
//}
