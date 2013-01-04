#pragma once

#include "util/callback.hpp"

namespace bklib {

class window;

namespace input {
namespace ime {

//! Container type for strings.
typedef std::vector<utf8string> string_container;

//------------------------------------------------------------------------------
//! Represents the current state of the IME candidate list.
//------------------------------------------------------------------------------
struct candidate_list {
    //! Number of list items to display at one time. Hardcoded to 9.
    //! @todo calculate at runtime
    static unsigned const ITEMS_PER_PAGE = 9;

    typedef string_container::const_iterator const_iterator;

    //! Default constructor.
    candidate_list()
        : current_index_(0)
        , current_page_(0)
        , page_count_(0)
        , items_per_page_(ITEMS_PER_PAGE)
    {
    }

    //! Construct from a container_t
    explicit candidate_list(
        string_container&& strings,
        unsigned      selection      = 0,
        unsigned      page           = 0,
        unsigned      items_per_page = ITEMS_PER_PAGE
    ) : current_index_(selection)
      , current_page_(page)
      , items_per_page_(items_per_page)
      , page_count_(0)
      , items_(std::move(strings))
    {
    }

    //! Iterator to the start of the current page.
    const_iterator page_begin() const { return items_.cbegin() + page_begin_index(); }
    //! Iterator to the end of the current page.
    const_iterator page_end()   const { return items_.cbegin() + page_end_index(); }

    unsigned items_per_page() const {
        return items_per_page_;
    }
    
    unsigned page_count() const {
        return page_count_;
    }

    //! Get the index of the start of the current page.
    unsigned page_begin_index() const {
        return std::min(count(), page() * items_per_page());
    }

    //! Get the index of the end of the current page.
    unsigned page_end_index() const {
        return std::min(count(), page_begin_index() + items_per_page());
    }

    //! Offset of the current selection relative to the start of the
    //! current page.
    unsigned page_offset() const {
        return current_index_ - page() * items_per_page();
    }

    unsigned count() const {
        return static_cast<unsigned>(items_.size());
    }

    unsigned page() const {
        return current_page_;
    }

    void set_items_per_page(unsigned n) {
        items_per_page_ = n;

        page_count_ = (count() / items_per_page()) +
            (count() % items_per_page() ? 1 : 0);
    }

    void set_page(unsigned page) {
        current_page_ = page;
    }

    void set_sel(unsigned index) {
        current_index_ = index;
    }

    //! Take ownership of the strings (move).
    void set_items(string_container&& strings) {
        items_ = std::move(strings);

        page_count_ = (count() / items_per_page()) +
            (count() % items_per_page() ? 1 : 0);
    }

    unsigned    current_index_;
    unsigned    current_page_;
    unsigned    items_per_page_;
    unsigned    page_count_;
    string_container items_;
}; //struct candidate_list
////////////////////////////////////////////////////////////////////////////////

enum class conversion_mode {
    full_hiragana,
    full_katakana,
    full_roman,
    half_katakana,
    half_roman,
};

enum class sentence_mode {
    general,
    speech,
    names,
    none,
};

namespace composition {
    enum class attribute {
        input,                  //! range is unconverted input
        target_converted,       //! range is current target and converted
        converted,              //! range has been converted
        target_not_converted,   //! range is current target and unconverted
        input_error,            //! range has an error
        fixed_conversion,       //! ?
        other                   //! ?
    };

    enum class line_style {
        none, solid, dot, dash, squiggle
    };

    //! describes a range of text
    struct range {
        utf8string text;
        attribute  attr;
        line_style ls;
    };

    typedef std::vector<range> range_list;
} //namespace composition

//------------------------------------------------------------------------------
//! Control and interact with the system IME. Implementation is platform
//! dependant: pimpl idiom.
//------------------------------------------------------------------------------
class manager {
public:
    manager();
    ~manager();

    void do_pending_events();
    void run(); //to be called only by the thread that also controls the system event loop

    void associate(window& window);

    void set_text(utf8string const& string);
    void cancel_composition();

    void capture_input(bool capture = true);

    //--------------------------------------------------------------------------
    BK_UTIL_CALLBACK_BEGIN;
        //----------------------------------------------------------------------
        //! Called when the active input language is changed.
        //! @li @a id
        //!     The locale identifier string.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_input_language_change,
                                  void (utf8string&& id) );
        //----------------------------------------------------------------------
        //! Called when the conversion mode is changed.
        //! @li @a mode
        //!     The new mode setting.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_input_conversion_mode_change,
                                  void (conversion_mode mode) );
        //----------------------------------------------------------------------
        //! Called when the sentence conversion mode is changed.
        //! @li @a mode
        //!     The new mode setting.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_input_sentence_mode_change,
                                  void (sentence_mode mode) );
        //----------------------------------------------------------------------
        //! Called when the IME is activated or deactivated.
        //! @li @a active
        //!     @c true when activated, @c false otherwise.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_input_activate,
                                  void (bool active) );
        //----------------------------------------------------------------------
        //! Called when a new input composition begins.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_composition_begin,
                                  void () );
        //----------------------------------------------------------------------
        //! Called when the active input composition is updated.
        //! @li @a ranges
        //!     The text ranges in the active composition and their attributes.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_composition_update,
                                  void (composition::range_list ranges) );
        //----------------------------------------------------------------------
        //! Called when the active input composition ends.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_composition_end,
                                  void () );
        //----------------------------------------------------------------------
        //! Called when the candidate list is opened.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_candidate_list_begin,
                                  void () );
        //----------------------------------------------------------------------
        //! Called when the current page of the candidate list changes.
        //! @li @a page
        //!     The new page index.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_candidate_list_change_page,
                                  void (unsigned page) );
        //----------------------------------------------------------------------
        //! Called when the current selection of the candidate list changes.
        //! @li @a selection
        //!     The new selection index.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_candidate_list_change_selection,
                                  void (unsigned selection) );
        //----------------------------------------------------------------------
        //! Called when the list of strings for the candidate list has changed.
        //! @li @a strings
        //!     The list of strings.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_candidate_list_change_strings,
                                  void (string_container&& strings) );
        //----------------------------------------------------------------------
        //! Called when the candidate list is closed.
        //----------------------------------------------------------------------
        BK_UTIL_CALLBACK_DECLARE( on_candidate_list_end,
                                  void () );
        //----------------------------------------------------------------------
    BK_UTIL_CALLBACK_END;
    //--------------------------------------------------------------------------
private:
    struct impl_t;
    std::unique_ptr<impl_t> impl_;
}; //class manager

BK_UTIL_CALLBACK_DECLARE_EXTERN(manager, on_input_language_change);
BK_UTIL_CALLBACK_DECLARE_EXTERN(manager, on_input_conversion_mode_change);
BK_UTIL_CALLBACK_DECLARE_EXTERN(manager, on_input_sentence_mode_change);
BK_UTIL_CALLBACK_DECLARE_EXTERN(manager, on_input_activate);
BK_UTIL_CALLBACK_DECLARE_EXTERN(manager, on_composition_begin);
BK_UTIL_CALLBACK_DECLARE_EXTERN(manager, on_composition_update);
BK_UTIL_CALLBACK_DECLARE_EXTERN(manager, on_composition_end);
BK_UTIL_CALLBACK_DECLARE_EXTERN(manager, on_candidate_list_begin);
BK_UTIL_CALLBACK_DECLARE_EXTERN(manager, on_candidate_list_change_page);
BK_UTIL_CALLBACK_DECLARE_EXTERN(manager, on_candidate_list_change_selection);
BK_UTIL_CALLBACK_DECLARE_EXTERN(manager, on_candidate_list_change_strings);
BK_UTIL_CALLBACK_DECLARE_EXTERN(manager, on_candidate_list_end);
////////////////////////////////////////////////////////////////////////////////

} //namespace ime
} //namespace input
} //namespace bklib
