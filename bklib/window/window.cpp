#include "pch.hpp"
#include "window.hpp"

#include "input/input.hpp"
#include "util/blocking_queue.hpp"

#include "platform/win/window.ipp"

//------------------------------------------------------------------------------
//! Windows implementation of bklib::window.
//------------------------------------------------------------------------------
struct bklib::window::impl_t
    : public bklib::detail::impl::window_impl
{
    typedef std::function<void ()>           message_t;
    typedef bklib::blocking_queue<message_t> queue_t;

    bool running_;
    std::unique_ptr<std::thread> thread_;

    impl_t(promise_t& finished, bklib::window& win)
        : window_impl()
        , running_(true)
    {
        thread_ = std::make_unique<std::thread>([&] {
            try {
                create();

                auto input_manager = std::make_shared<bklib::input::ime::manager>();
                input_manager->associate(win);

                finished.set_value(input_manager);
                
                do {
                    while (do_input_message()) {}
                    input_manager->run();
                } while (running_ && do_event_wait());
            } catch (bklib::exception_base&) {
                BK_TODO_BREAK;
            } catch (std::exception&) {
                BK_TODO_BREAK;
            } catch (...) {
                BK_TODO_BREAK;
            }
        });
    }

    template <typename T>
    void post_input_message(T msg) {
        input.emplace(msg);
        notfify();
    }

    bool do_input_message() {
        if (input.empty()) return false;

        input.pop()();
        return true;
    }

    template <typename T>
    void post_output_message(T msg) {
        output.emplace(msg);
    }

    bool do_output_message() {
        if (output.empty()) return false;

        output.pop()();
        return true;
    }

    queue_t input;
    queue_t output;
};
//------------------------------------------------------------------------------

bklib::window::window(promise_t& finished)
    : impl_(new impl_t(finished, *this))
{
}

bklib::window::~window()
{
}

void
bklib::window::wait() {
    impl_->thread_->join();
}

void bklib::window::close() {
    impl_->post_input_message([&] {
        impl_->close();
        impl_->running_ = false;
    });
}

void
bklib::window::show(bool visible) {
    impl_->post_input_message([&, visible] {
        impl_->show(visible);
    });
}

bool
bklib::window::has_pending_events() const {
    return !impl_->output.empty();
}

void
bklib::window::do_pending_events() {
    while (impl_->do_output_message()) {
    }
}

void
bklib::window::do_event_wait() {
    impl_->output.pop()();
}

bklib::window::handle_t
bklib::window::handle() const
{
    return impl_->handle();
}

BK_UTIL_CALLBACK_DEFINE_IMPL(bklib::window, on_key_down) {
    impl_->on_key_down = [&, handler](window::key_code_t key) {
        impl_->post_output_message([&, key] { handler(key); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(bklib::window, on_key_up) {
    impl_->on_key_up = [&, handler](window::key_code_t key) {
        impl_->post_output_message([&, key] { handler(key); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(bklib::window, on_mouse_move_to) {
    impl_->on_mouse_move_to = [&, handler](int x, int y) {
        impl_->output.emplace([&, x, y] { handler(x, y); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(bklib::window, on_close) {
    impl_->on_close = [&, handler] {
        impl_->post_output_message([&] { handler(); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(bklib::window, on_paint) {
    impl_->on_paint = [&, handler]() {
        impl_->output.emplace([&] { handler(); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(bklib::window, on_size) {
    impl_->on_size = [&, handler](unsigned w, unsigned h) {
        impl_->post_output_message([&, w, h] { handler(w, h); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(bklib::window, on_mouse_move) {
}

BK_UTIL_CALLBACK_DEFINE_IMPL(bklib::window, on_mouse_scroll) {
    impl_->on_mouse_scroll = [&, handler](signed ds) {
        impl_->post_output_message([&, ds] { handler(ds); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(bklib::window, on_mouse_down) {
    impl_->on_mouse_down = [&, handler](unsigned button) {
        impl_->post_output_message([&, button] { handler(button); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(bklib::window, on_mouse_up) {
    impl_->on_mouse_up = [&, handler](unsigned button) {
        impl_->post_output_message([&, button] { handler(button); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(bklib::window, on_input_char) {
    impl_->on_input_char = [&, handler](utf32codepoint cp) {
        impl_->post_output_message([&, cp] { handler(cp); });
    };
}
