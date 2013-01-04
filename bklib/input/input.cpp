#include "pch.hpp"
#include "input.hpp"

#include "util//blocking_queue.hpp"
#include "window/window.hpp"

////////////////////////////////////////////
#include "platform/win/input.ipp"
////////////////////////////////////////////

namespace ime = ::bklib::input::ime;

struct ime::manager::impl_t : public bklib::detail::impl::ime_manager_impl_t {
    typedef std::function<void ()>           message_t;
    typedef bklib::blocking_queue<message_t> queue_t;

    impl_t(ime::manager& manager) : ime_manager_impl_t(manager) {
    }

    queue_t input;
    queue_t output;
};

ime::manager::manager()
    : impl_(std::make_unique<impl_t>(*this))
{
}

ime::manager::~manager()
{
}

void
ime::manager::do_pending_events()
{
    while (!impl_->output.empty()) {
        impl_->output.pop()();
    }
}

void
ime::manager::run()
{
    while (!impl_->input.empty()) {
        impl_->input.pop()();
    }
}

void
ime::manager::associate(bklib::window& window) {
    auto const handle = window.handle();

    impl_->input.emplace([&, handle] {
        impl_->associate(handle);
    });
    impl_->notify();
}

void
ime::manager::set_text(bklib::utf8string const& string) {
    impl_->input.emplace([&, string] {
        impl_->set_text(string);
    });
    impl_->notify();
}

void
ime::manager::cancel_composition() {
    impl_->input.emplace([&] {
        impl_->cancel_composition();
    });
    impl_->notify();
}

void
ime::manager::capture_input(bool capture) {
    impl_->input.emplace([&, capture] {
        impl_->capture_input(capture);
    });
}

BK_UTIL_CALLBACK_DEFINE_IMPL(ime::manager, on_input_language_change) {
    impl_->on_input_language_change = [&, handler](utf8string&& id) {
        impl_->output.emplace([&, id]() mutable { handler(std::move(id)); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(ime::manager, on_input_conversion_mode_change) {
    impl_->on_input_conversion_mode_change = [&, handler](conversion_mode mode) {
        impl_->output.emplace([&, mode] { handler(mode); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(ime::manager, on_input_sentence_mode_change) {
    impl_->on_input_sentence_mode_change = [&, handler](sentence_mode mode) {
        impl_->output.emplace([&, mode] { handler(mode); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(ime::manager, on_input_activate) {
    impl_->on_input_activate = [&, handler](bool active) {
        impl_->output.emplace([&, active] { handler(active); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(ime::manager, on_composition_begin) {
    impl_->on_composition_begin = [&, handler] {
        impl_->output.emplace([&] { handler(); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(ime::manager, on_composition_update) {
    //unneeded copy...
    impl_->on_composition_update = [&, handler](ime::composition::range_list ranges) {
        impl_->output.emplace([&, ranges] { handler(std::move(ranges)); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(ime::manager, on_composition_end) {
    impl_->on_composition_end = [&, handler] {
        impl_->output.emplace([&] { handler(); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(ime::manager, on_candidate_list_begin) {
    impl_->on_candidate_list_begin = [&, handler] {
        impl_->output.emplace([&] { handler(); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(ime::manager, on_candidate_list_change_page) {
    impl_->on_candidate_list_change_page = [&, handler](unsigned page) {
        impl_->output.emplace([&, page] { handler(page); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(ime::manager, on_candidate_list_change_selection) {
    impl_->on_candidate_list_change_selection = [&, handler](unsigned selection) {
        impl_->output.emplace([&, selection] { handler(selection); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(ime::manager, on_candidate_list_change_strings) {
    impl_->on_candidate_list_change_strings = [&, handler](std::vector<utf8string>&& strings) {
        impl_->output.emplace([&, strings]() mutable { handler(std::move(strings)); });
    };
}

BK_UTIL_CALLBACK_DEFINE_IMPL(ime::manager, on_candidate_list_end) {
    impl_->on_candidate_list_end = [&, handler] {
        impl_->output.emplace([&] { handler(); });
    };
}
