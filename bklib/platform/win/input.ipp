//------------------------------------------------------------------------------
//! @file
//! @author Brandon Kentel
//! @date   Dec 2012
//! @brief  Windows implementation of IME control. Implemented via the Text
//!         Services Framework.
//------------------------------------------------------------------------------

#include "pch.hpp"

#include "input/input.hpp"
#include "com/com.hpp"

#pragma warning( disable : 4100 ) //! @todo unreferenced params
#pragma warning( disable : 4702 ) //! @todo unreachable code

//#define BK_TRACE_BEGIN ::OutputDebugStringW(__FUNCTIONW__ L"\n");
#define BK_TRACE_BEGIN
#define BK_TRACE_VAR(x)
#define BK_TRACE_END

namespace win = ::bklib::platform::win;
namespace ime = ::bklib::input::ime;

namespace bklib {
namespace detail {
namespace impl {

//------------------------------------------------------------------------------
//! Translates the currently active exception to an HRESULT, otherwise does
//! nothing.
//------------------------------------------------------------------------------
HRESULT translate_exception() BK_NOEXCEPT {
    auto e = std::current_exception();
    if (!e) return S_OK;

    try {
        std::rethrow_exception(e);
    } catch (bklib::platform::com_exception& e) {
        auto hr = boost::get_error_info<bklib::platform::com_result_code>(e);
        return hr ? *hr : E_FAIL;
    } catch (bklib::exception_base&) {
        return E_FAIL;
    } catch (std::exception&) {
        return E_FAIL;
    } catch (...) {
        return E_FAIL;
    }

    return E_FAIL; // workaround for a spurious warning
}

//! Helper macro to ensure no exceptions escape across the API boundary.
#define BK_TRANSLATE_EXCEPTIONS_BEGIN try
//! Helper macro to ensure no exceptions escape across the API boundary.
#define BK_TRANSLATE_EXCEPTIONS_END   catch (...) { return translate_exception(); }

//------------------------------------------------------------------------------
//! Maintains a reference to a ITfSource object, and list of sink<->cookie
//! records.
//------------------------------------------------------------------------------
struct source_connection {
    typedef std::pair<IID, DWORD> record_t;

    source_connection() { }

    template <typename T>
    source_connection(win::com_ptr<T>& ptr)
        : source(win::make_com_ptr([&](ITfSource** out) {
            return ptr->QueryInterface<ITfSource>(out);
        }))
    {
    }

    template <typename T>
    HRESULT advise(T* ptr) {
        record_t r = {__uuidof(T), 0};
        HRESULT const hr = source->AdviseSink(__uuidof(T), ptr, &r.second);

        if (SUCCEEDED(hr)) {
            records_.emplace_back(std::move(r));
        }

        return hr;
    }

    win::com_ptr<ITfSource> source;
    std::vector<record_t>   records_;
}; //---------------------------------------------------------------------------

//------------------------------------------------------------------------------
//! Encapuslates the information required to track an advise sink.
//------------------------------------------------------------------------------
struct advise_sink_t {
    advise_sink_t() : key(nullptr), flags(0) {}

    //! The value of the IUnknown pointer is used as unique identifier.
    IUnknown*                       key;
    win::com_ptr<ITextStoreACPSink> sink;
    DWORD                           flags;
}; //---------------------------------------------------------------------------

//------------------------------------------------------------------------------
//! Helper for TSF lock state.
//------------------------------------------------------------------------------
struct lock_t {
    lock_t() : flags_(0) {}

    bool is_sync()   const { return (flags_ & TS_LF_SYNC) != 0; }
    bool is_read()   const { return (flags_ & TS_LF_READ) != 0; }
    bool is_write()  const { return (flags_ & TS_LF_READWRITE) != 0; }
    bool is_locked() const { return flags_ != 0; }

    void lock(DWORD flags) { flags_ = flags; }
    void unlock() { flags_ = 0; }

    DWORD flags_;
}; //---------------------------------------------------------------------------

//------------------------------------------------------------------------------
//! Helper for text selections. Merely wraps a TS_SELECTION_ACP and adds a few
//! helper functions.
//------------------------------------------------------------------------------
struct selection_t : public TS_SELECTION_ACP {
    selection_t() : selection_t(0, 0, TS_AE_END, FALSE) {
    }

    selection_t(LONG start, LONG end, TsActiveSelEnd ase, BOOL interim)
        : TS_SELECTION_ACP(init_(start, end, ase, interim))
    {
    }

    ULONG length() const {
        return acpEnd - acpStart;
    }

    bool is_caret() const {
        return length() == 0;
    }

    //------------------------------------------------------------------------------
    //! Helper to fill in a TS_SELECTION_ACP struct.
    //------------------------------------------------------------------------------
    static TS_SELECTION_ACP init_(
        LONG start, LONG end,
        TsActiveSelEnd ase, BOOL interim
    ) {
        TS_SELECTION_ACP const result {
            start, end, {ase, interim}
        };

        return result;
    }
}; //---------------------------------------------------------------------------

//------------------------------------------------------------------------------
//! Manages the state of the IME composition list.
//------------------------------------------------------------------------------
struct candidate_list {
    candidate_list(DWORD id = 0)
        : element_id(id), page(0), selection(0), count(0)
    {
    }

    //------------------------------------------------------------------------------
    //! Get an ITfCandidateListUIElement from @c manager, otherwise fail.
    //! @param id Obtained from one of the callbacks from @c ITfUIElementSink
    //! @throw platform::windows_exception
    //------------------------------------------------------------------------------
    candidate_list(ITfUIElementMgr& manager, DWORD id) : candidate_list(id) {
        ui = win::make_com_ptr([&](ITfCandidateListUIElement** out) {
            auto const hr = win::make_com_ptr([&](ITfUIElement** out) {
                return manager.GetUIElement(element_id, out);
            })->QueryInterface(out);

            BK_THROW_ON_FAIL(::ITfUIElementMgr::GetUIElement, hr);

            return hr;
        });
    }

    //------------------------------------------------------------------------------
    //! Update based on the @c flags from @c ITfCandidateListUIElement.
    //! @return The update flags.
    //! @throw platform::windows_exception
    //------------------------------------------------------------------------------
    DWORD update() {
        DWORD flags = 0;
        auto const hr = ui->GetUpdatedFlags(&flags);
        BK_THROW_ON_FAIL(::ITfCandidateListUIElement::GetUpdatedFlags, hr);

        if (flags & TF_CLUIE_DOCUMENTMGR) {
            BK_TODO_BREAK; //! @todo handle this case.
        }
        if (flags & TF_CLUIE_COUNT) {
            auto const hr = ui->GetCount(&count);
            BK_THROW_ON_FAIL(::ITfCandidateListUIElement::GetCount, hr);
        }
        if (flags & TF_CLUIE_SELECTION) {
            auto const hr = ui->GetSelection(&selection);
            BK_THROW_ON_FAIL(::ITfCandidateListUIElement::GetSelection, hr);
        }
        if (flags & TF_CLUIE_STRING) {
            get_strings_();
        }
        if (flags & TF_CLUIE_PAGEINDEX) {
            get_pages_();
        }
        if (flags & TF_CLUIE_CURRENTPAGE) {
            auto const hr = ui->GetCurrentPage(&page);
            BK_THROW_ON_FAIL(::ITfCandidateListUIElement::GetCurrentPage, hr);
        }

        return flags;
    }

    std::vector<utf8string> get_utf8_strings() const {
        std::vector<utf8string> result;
        result.reserve(strings.size());

        for (auto const& i : strings) {
            result.emplace_back(convert.to_bytes(i));
        }

        return result;
    }

    UINT                      page;
    UINT                      selection;
    UINT                      count;
    std::vector<std::wstring> strings;
    std::vector<UINT>         pages;
private:
    //------------------------------------------------------------------------------
    //! Retrieve the list of page sizes.
    //! @throw platform::windows_exception
    //------------------------------------------------------------------------------
    void get_pages_() {
        UINT page_count = 0;
        auto const hr1 = ui->GetPageIndex(nullptr, 0, &page_count);
        BK_THROW_ON_FAIL(::ITfUIElementMgr::GetPageIndex, hr1);

        pages.resize(page_count, 0);
        
        auto const hr2 = ui->GetPageIndex(pages.data(), pages.size(), &page_count);
        BK_THROW_ON_FAIL(::ITfUIElementMgr::GetPageIndex, hr2);
    }

    //------------------------------------------------------------------------------
    //! Retrieve the list of candidate strings.
    //! @throw platform::windows_exception
    //------------------------------------------------------------------------------
    void get_strings_() {
        strings.reserve(count);

        for (UINT i = 0; i < count; ++i) {
            BSTR str;
            
            // make sure to clean up.
            BK_ON_SCOPE_EXIT({
                ::SysFreeString(str);
            });

            auto const hr = ui->GetString(i, &str);
            BK_THROW_ON_FAIL(::ITfUIElementMgr::GetString, hr);

            strings.emplace_back(str);
        }
    }

    win::com_ptr<ITfCandidateListUIElement> ui;

    DWORD element_id;

    bklib::utf8_16_converter mutable convert; //!< Used by get_utf8_strings.
}; //---------------------------------------------------------------------------

//------------------------------------------------------------------------------
//! Windows implementation of input::ime::manager.
//------------------------------------------------------------------------------
struct ime_manager_impl_t
    : public ITextStoreACP
    , public ITfContextOwnerCompositionSink
    , public ITfLanguageProfileNotifySink
    , public ITfActiveLanguageProfileNotifySink
    , public ITfThreadMgrEventSink
    , public ITfCompartmentEventSink
    , public ITfUIElementSink
{
public:
    ime::manager::event_on_input_language_change::type           on_input_language_change;
    ime::manager::event_on_input_conversion_mode_change::type    on_input_conversion_mode_change;
    ime::manager::event_on_input_sentence_mode_change::type      on_input_sentence_mode_change;
    ime::manager::event_on_input_activate::type                  on_input_activate;

    ime::manager::event_on_composition_begin::type               on_composition_begin;
    ime::manager::event_on_composition_update::type              on_composition_update;
    ime::manager::event_on_composition_end::type                 on_composition_end;

    ime::manager::event_on_candidate_list_begin::type            on_candidate_list_begin;
    ime::manager::event_on_candidate_list_change_page::type      on_candidate_list_change_page;
    ime::manager::event_on_candidate_list_change_selection::type on_candidate_list_change_selection;
    ime::manager::event_on_candidate_list_change_strings::type   on_candidate_list_change_strings;
    ime::manager::event_on_candidate_list_end::type              on_candidate_list_end;
public:
    ////////////////////////////////////////////////////////////////////////////
    // IUnknown
    ////////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID riid,
        _COM_Outptr_ void **ppvObject
    );

    ULONG STDMETHODCALLTYPE AddRef()  { return ++ref_count_; }
    ULONG STDMETHODCALLTYPE Release() { return --ref_count_; }
    ////////////////////////////////////////////////////////////////////////////
    // ITfUIElementSink
    ////////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE
    BeginUIElement(DWORD dwUIElementId, BOOL *pbShow);

    HRESULT STDMETHODCALLTYPE
    UpdateUIElement(DWORD dwUIElementId);

    HRESULT STDMETHODCALLTYPE
    EndUIElement(DWORD dwUIElementId);
    ////////////////////////////////////////////////////////////////////////////
    // ITfContextOwnerCompositionSink
    ////////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE OnStartComposition(
        __RPC__in_opt ITfCompositionView *pComposition,
        __RPC__out BOOL *pfOk);

    HRESULT STDMETHODCALLTYPE OnUpdateComposition(
        __RPC__in_opt ITfCompositionView *pComposition,
        __RPC__in_opt ITfRange *pRangeNew);

    HRESULT STDMETHODCALLTYPE OnEndComposition(
        __RPC__in_opt ITfCompositionView *pComposition);
    ////////////////////////////////////////////////////////////////////////////
    // ITfLanguageProfileNotifySink
    ////////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE OnLanguageChange(
        LANGID langid,
        __RPC__out BOOL *pfAccept);

    HRESULT STDMETHODCALLTYPE OnLanguageChanged();
    ////////////////////////////////////////////////////////////////////////////
    // ITfActiveLanguageProfileNotifySink
    ////////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE OnActivated(
        __RPC__in REFCLSID clsid,
        __RPC__in REFGUID guidProfile,
        BOOL fActivated);
    ////////////////////////////////////////////////////////////////////////////
    // ITfThreadMgrEventSink
    ////////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE OnInitDocumentMgr(
        __RPC__in_opt ITfDocumentMgr *pdim);

    HRESULT STDMETHODCALLTYPE OnUninitDocumentMgr(
        __RPC__in_opt ITfDocumentMgr *pdim);

    HRESULT STDMETHODCALLTYPE OnSetFocus(
        __RPC__in_opt ITfDocumentMgr *pdimFocus,
        __RPC__in_opt ITfDocumentMgr *pdimPrevFocus);

    HRESULT STDMETHODCALLTYPE OnPushContext(
        __RPC__in_opt ITfContext *pic);

    HRESULT STDMETHODCALLTYPE OnPopContext(
        __RPC__in_opt ITfContext *pic);
    ////////////////////////////////////////////////////////////////////////////
    // ITfCompartmentEventSink
    ////////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE OnChange(__RPC__in REFGUID rguid);
    ////////////////////////////////////////////////////////////////////////////
    // ITextStoreACP
    ////////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE AdviseSink(
        __RPC__in REFIID riid,
        __RPC__in_opt IUnknown *punk,
        DWORD dwMask);

    HRESULT STDMETHODCALLTYPE UnadviseSink(
        __RPC__in_opt IUnknown *punk);

    HRESULT STDMETHODCALLTYPE RequestLock(
        DWORD dwLockFlags,
        __RPC__out HRESULT *phrSession);

    HRESULT STDMETHODCALLTYPE GetStatus(
        __RPC__out TS_STATUS *pdcs);

    HRESULT STDMETHODCALLTYPE QueryInsert(
        LONG acpTestStart,
        LONG acpTestEnd,
        ULONG cch,
        __RPC__out LONG *pacpResultStart,
        __RPC__out LONG *pacpResultEnd);

    HRESULT STDMETHODCALLTYPE GetSelection(
        ULONG ulIndex,
        ULONG ulCount,
        __RPC__out_ecount_part(ulCount, *pcFetched) TS_SELECTION_ACP *pSelection,
        __RPC__out ULONG *pcFetched);

    HRESULT STDMETHODCALLTYPE SetSelection(
        ULONG ulCount,
        __RPC__in_ecount_full(ulCount) const TS_SELECTION_ACP *pSelection);

    HRESULT STDMETHODCALLTYPE GetText(
        LONG acpStart,
        LONG acpEnd,
        __RPC__out_ecount_part(cchPlainReq, *pcchPlainRet) WCHAR *pchPlain,
        ULONG cchPlainReq,
        __RPC__out ULONG *pcchPlainRet,
        __RPC__out_ecount_part(cRunInfoReq, *pcRunInfoRet) TS_RUNINFO *prgRunInfo,
        ULONG cRunInfoReq,
        __RPC__out ULONG *pcRunInfoRet,
        __RPC__out LONG *pacpNext);

    HRESULT STDMETHODCALLTYPE SetText(
        DWORD dwFlags,
        LONG acpStart,
        LONG acpEnd,
        __RPC__in_ecount_full(cch) const WCHAR *pchText,
        ULONG cch,
        __RPC__out TS_TEXTCHANGE *pChange);

    HRESULT STDMETHODCALLTYPE GetFormattedText(
        LONG acpStart,
        LONG acpEnd,
        __RPC__deref_out_opt IDataObject **ppDataObject);

    HRESULT STDMETHODCALLTYPE GetEmbedded(
        LONG acpPos,
        __RPC__in REFGUID rguidService,
        __RPC__in REFIID riid,
        __RPC__deref_out_opt IUnknown **ppunk);

    HRESULT STDMETHODCALLTYPE QueryInsertEmbedded(
        __RPC__in const GUID *pguidService,
        __RPC__in const FORMATETC *pFormatEtc,
        __RPC__out BOOL *pfInsertable);

    HRESULT STDMETHODCALLTYPE InsertEmbedded(
        DWORD dwFlags,
        LONG acpStart,
        LONG acpEnd,
        __RPC__in_opt IDataObject *pDataObject,
        __RPC__out TS_TEXTCHANGE *pChange);

    HRESULT STDMETHODCALLTYPE InsertTextAtSelection(
        DWORD dwFlags,
        __RPC__in_ecount_full(cch) const WCHAR *pchText,
        ULONG cch,
        __RPC__out LONG *pacpStart,
        __RPC__out LONG *pacpEnd,
        __RPC__out TS_TEXTCHANGE *pChange);

    HRESULT STDMETHODCALLTYPE InsertEmbeddedAtSelection(
        DWORD dwFlags,
        __RPC__in_opt IDataObject *pDataObject,
        __RPC__out LONG *pacpStart,
        __RPC__out LONG *pacpEnd,
        __RPC__out TS_TEXTCHANGE *pChange);

    HRESULT STDMETHODCALLTYPE RequestSupportedAttrs(
        DWORD dwFlags,
        ULONG cFilterAttrs,
        __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID *paFilterAttrs);

    HRESULT STDMETHODCALLTYPE RequestAttrsAtPosition(
        LONG acpPos,
        ULONG cFilterAttrs,
        __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID *paFilterAttrs,
        DWORD dwFlags);

    HRESULT STDMETHODCALLTYPE RequestAttrsTransitioningAtPosition(
        LONG acpPos,
        ULONG cFilterAttrs,
        __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID *paFilterAttrs,
        DWORD dwFlags);

    HRESULT STDMETHODCALLTYPE FindNextAttrTransition(
        LONG acpStart,
        LONG acpHalt,
        ULONG cFilterAttrs,
        __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID *paFilterAttrs,
        DWORD dwFlags,
        __RPC__out LONG *pacpNext,
        __RPC__out BOOL *pfFound,
        __RPC__out LONG *plFoundOffset);

    HRESULT STDMETHODCALLTYPE RetrieveRequestedAttrs(
        ULONG ulCount,
        __RPC__out_ecount_part(ulCount, *pcFetched) TS_ATTRVAL *paAttrVals,
        __RPC__out ULONG *pcFetched);

    HRESULT STDMETHODCALLTYPE GetEndACP(
        __RPC__out LONG *pacp);

    HRESULT STDMETHODCALLTYPE GetActiveView(
        __RPC__out TsViewCookie *pvcView);

    HRESULT STDMETHODCALLTYPE GetACPFromPoint(
        TsViewCookie vcView,
        __RPC__in const POINT *ptScreen,
        DWORD dwFlags,
        __RPC__out LONG *pacp);

    HRESULT STDMETHODCALLTYPE GetTextExt(
        TsViewCookie vcView,
        LONG acpStart,
        LONG acpEnd,
        __RPC__out RECT *prc,
        __RPC__out BOOL *pfClipped);

    HRESULT STDMETHODCALLTYPE GetScreenExt(
        TsViewCookie vcView,
        __RPC__out RECT *prc);

    HRESULT STDMETHODCALLTYPE GetWnd(
        TsViewCookie vcView,
        __RPC__deref_out_opt HWND *phwnd);
public:
    ime_manager_impl_t(ime::manager& manager);

    void associate(HWND window);

    void capture_input(bool capture) {
        HRESULT hr = S_OK;

        if (capture != keystroke_feed_enabled_) {
            keystroke_feed_enabled_ = capture;

            auto const hr = capture ?
                sys_key_feed_->EnableSystemKeystrokeFeed() :
                sys_key_feed_->DisableSystemKeystrokeFeed();

            BK_ASSERT(SUCCEEDED(hr));
        }
    }

    void set_text(utf8string const& string) {
        //utf8_16_converter convert;

        text_ = converter_.from_bytes(string);

        TS_TEXTCHANGE const change = {
            0,
            selection_.acpEnd,
            text_.size()
        };

        if (FAILED(advise_sink_.sink->OnTextChange(0, &change))) {
            BK_TODO_BREAK;
        }

        selection_.acpStart = text_.size();
        selection_.acpEnd   = text_.size();
    }

    void cancel_composition() {
        //This causes an error TODO!
        HRESULT hr = win::make_com_ptr([&](ITfContextOwnerCompositionServices** out) {
            return context_->QueryInterface(out);
        })->TerminateComposition(nullptr);

        auto context = win::make_com_ptr([&](ITfContext** out) {
            return document_manager_->GetTop(out);
        });

        hr = win::make_com_ptr([&](ITfContextOwnerCompositionServices** out) {
            return context->QueryInterface(out);
        })->TerminateComposition(nullptr);
    }

    void notify() {
        PostMessage(window_, WM_USER, 0, 0);
    }

    utf8string get_input_language() const {
        return converter_.to_bytes(input_language_id_);
    }
private:
    ime_manager_impl_t(ime_manager_impl_t const&); //=delete
    ime_manager_impl_t& operator=(ime_manager_impl_t const&); //=delete

    ime::manager& manager_;

    ULONG ref_count_;

    TfClientId   client_id_;
    TfEditCookie edit_cookie_;

    win::com_ptr<ITfThreadMgr>                    thread_manager_;
    win::com_ptr<ITfDocumentMgr>                  document_manager_;
    win::com_ptr<ITfContext>                      context_;
    win::com_ptr<ITfDisplayAttributeMgr>          attr_manager_;
    win::com_ptr<ITfCategoryMgr>                  cat_manager_;
    win::com_ptr<ITfCompartmentMgr>               compartment_manager_;
    win::com_ptr<ITfUIElementMgr>                 ui_manager_;
    win::com_ptr<ITfInputProcessorProfiles>       input_profiles_;
    win::com_ptr<ITfConfigureSystemKeystrokeFeed> sys_key_feed_;

    source_connection ui_manager_connection_;
    source_connection input_profiles_connection_;
    source_connection thread_manager_connection_;
    source_connection input_conversion_connection_;
    source_connection input_sentence_connection_;
    source_connection input_openclose_connection_;

    struct conversion_status_t {
        conversion_status_t()
            : active(0), conversion_mode(0), sentence_mode(0)
        {
        }

        DWORD active;
        DWORD conversion_mode;
        DWORD sentence_mode;
    } conversion_status_;

    HWND           window_;
    TsViewCookie   view_cookie_;
    advise_sink_t  advise_sink_;
    selection_t    selection_;
    lock_t         lock_;
    std::wstring   text_;
    candidate_list candidates_;
    bool           keystroke_feed_enabled_;

    std::wstring input_language_id_;

    utf8_16_converter mutable converter_;
};
////////////////////////////////////////////////////////////////////////////////

} //namespace bklib
} //namespace detail
} //namespace impl

namespace ime = bklib::input::ime;
namespace impl = bklib::detail::impl;

impl::ime_manager_impl_t::ime_manager_impl_t(ime::manager& manager)
    : manager_(manager)
    , ref_count_(0)
    , window_(0)
    , view_cookie_(1984)
    , keystroke_feed_enabled_(true)
{
    ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);

    thread_manager_ = win::make_com_ptr([](ITfThreadMgr** out) {
        return ::CoCreateInstance(
            CLSID_TF_ThreadMgr,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_ITfThreadMgr,
            (void**)out
        );
    });

    cat_manager_ = win::make_com_ptr([&](ITfCategoryMgr** out) {
        return ::CoCreateInstance(
            CLSID_TF_CategoryMgr,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_ITfCategoryMgr,
            (void**)out
        );
    });
     
    attr_manager_ = win::make_com_ptr([&](ITfDisplayAttributeMgr** out) {
        return ::CoCreateInstance(
            CLSID_TF_DisplayAttributeMgr,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_ITfDisplayAttributeMgr,
            (void**)out
        );
    });

    input_profiles_ = win::make_com_ptr([&](ITfInputProcessorProfiles** out) {
        return ::CoCreateInstance(
            CLSID_TF_InputProcessorProfiles,
            nullptr,
            CLSCTX_INPROC_SERVER,
            IID_ITfInputProcessorProfiles,
            (void**)out
        );
    });

    sys_key_feed_ = win::make_com_ptr([&](ITfConfigureSystemKeystrokeFeed** out) {
        return thread_manager_->QueryInterface(out);
    });

    ui_manager_ = win::make_com_ptr([&](ITfUIElementMgr** out) {
        return thread_manager_->QueryInterface(out);
    });

    ui_manager_connection_ = source_connection(ui_manager_);
    ui_manager_connection_.advise<ITfUIElementSink>(this);

    ////////////////////////////////////////////////////////////////////////////
    compartment_manager_ = win::make_com_ptr([&](ITfCompartmentMgr** out) {
        return thread_manager_->QueryInterface<ITfCompartmentMgr>(out);
    });

    {
        auto compartment = win::make_com_ptr([&](ITfCompartment** out) {
            return compartment_manager_->GetCompartment(GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION, out);
        });

        input_conversion_connection_ = source_connection(compartment);
        input_conversion_connection_.advise<ITfCompartmentEventSink>(this);
    }
    {
        auto compartment = win::make_com_ptr([&](ITfCompartment** out) {
            return compartment_manager_->GetCompartment(GUID_COMPARTMENT_KEYBOARD_INPUTMODE_SENTENCE, out);
        });

        input_sentence_connection_ = source_connection(compartment);
        input_sentence_connection_.advise<ITfCompartmentEventSink>(this);
    }
    {
        auto compartment = win::make_com_ptr([&](ITfCompartment** out) {
            return compartment_manager_->GetCompartment(GUID_COMPARTMENT_KEYBOARD_OPENCLOSE, out);
        });

        input_openclose_connection_ = source_connection(compartment);
        input_openclose_connection_.advise<ITfCompartmentEventSink>(this);
    }
    ////////////////////////////////////////////////////////////////////////////

    thread_manager_connection_ = source_connection(thread_manager_);
    thread_manager_connection_.advise<ITfThreadMgrEventSink>(this);
    thread_manager_connection_.advise<ITfActiveLanguageProfileNotifySink>(this);

    input_profiles_connection_ = source_connection(input_profiles_);
    input_profiles_connection_.advise<ITfLanguageProfileNotifySink>(this);

    //! @todo move this to a seperate method... 
    if (FAILED(thread_manager_->Activate(&client_id_))) {
        return;
    }

    document_manager_ = win::make_com_ptr([&](ITfDocumentMgr** out) {
        return thread_manager_->CreateDocumentMgr(out);
    });

    context_ = win::make_com_ptr([&](ITfContext** out) {
        return document_manager_->CreateContext(client_id_, 0, static_cast<ITextStoreACP*>(this), out, &edit_cookie_);
    });

    if (FAILED(document_manager_->Push(context_))) {
        return;
    }
}

void
impl::ime_manager_impl_t::associate(HWND window) {
    ITfDocumentMgr* prev_doc = nullptr;
    if (FAILED(thread_manager_->AssociateFocus(window, document_manager_, &prev_doc))) {
        return;
    }

    capture_input(false);

    window_ = window;
}

////////////////////////////////////////////////////////////////////////////////
// IUnknown
////////////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::QueryInterface(
    /* [in] */ REFIID riid,
    /* [iid_is][out] */ _COM_Outptr_ void **ppvObject
) {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(riid)
        BK_TRACE_VAR(ppvObject)
    BK_TRACE_END

    if (nullptr == ppvObject) {
        return E_INVALIDARG;
    }

    if (IsEqualIID(riid, __uuidof(IUnknown))) {
        *ppvObject = static_cast<IUnknown*>(
            static_cast<ITextStoreACP*>(this)
        );
    } else if (IsEqualIID(riid, __uuidof(ITextStoreACP))) {
        *ppvObject = static_cast<ITextStoreACP*>(this);
    } else if (IsEqualIID(riid, __uuidof(ITfContextOwnerCompositionSink))) {
        *ppvObject = static_cast<ITfContextOwnerCompositionSink*>(this);
    }  else if (IsEqualIID(riid, __uuidof(ITfLanguageProfileNotifySink))) {
        *ppvObject = static_cast<ITfLanguageProfileNotifySink*>(this);
    } else if (IsEqualIID(riid, __uuidof(ITfActiveLanguageProfileNotifySink))) {
        *ppvObject = static_cast<ITfActiveLanguageProfileNotifySink*>(this);
    } else if (IsEqualIID(riid, __uuidof(ITfThreadMgrEventSink))) {
        *ppvObject = static_cast<ITfThreadMgrEventSink*>(this);
    } else if (IsEqualIID(riid, __uuidof(ITfCompartmentEventSink))) {
        *ppvObject = static_cast<ITfCompartmentEventSink*>(this);
    } else if (IsEqualIID(riid, __uuidof(ITfUIElementSink))) {
        *ppvObject = static_cast<ITfUIElementSink*>(this);
    } else {
        return E_NOINTERFACE;
    }

    AddRef();

    return S_OK;
}

//------------------------------------------------------------------------------
//! Called when a UI element is about to be shown.
//! @throw noexcept
//------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::BeginUIElement(DWORD dwUIElementId, BOOL* pbShow)
BK_TRANSLATE_EXCEPTIONS_BEGIN {
    BK_TRACE_BEGIN
    BK_TRACE_END

    // use the system ui.
    *pbShow = TRUE;

    candidates_ = candidate_list(*ui_manager_, dwUIElementId);

    if (on_candidate_list_begin) {
        on_candidate_list_begin();
        *pbShow = FALSE; // use a custom drawn ui.
    }

    return S_OK;
} BK_TRANSLATE_EXCEPTIONS_END

//------------------------------------------------------------------------------
//! Called when a UI element has been updated in any way.
//! @throw noexcept
//------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::UpdateUIElement(DWORD dwUIElementId)
BK_TRANSLATE_EXCEPTIONS_BEGIN {
    BK_TRACE_BEGIN
    BK_TRACE_END

    auto const flags = candidates_.update();

    if (flags & TF_CLUIE_DOCUMENTMGR) {
        BK_TODO_BREAK; //! @todo handle this flag.
    }
    if (flags & TF_CLUIE_COUNT) {
    }
    if (flags & TF_CLUIE_SELECTION) {
        safe_callback(on_candidate_list_change_selection, candidates_.selection);
    }
    if (flags & TF_CLUIE_STRING) {
        if (on_candidate_list_change_strings) {
            on_candidate_list_change_strings(candidates_.get_utf8_strings());
        }
    }
    if (flags & TF_CLUIE_PAGEINDEX) {
        //! @todo handle this flag.
    }
    if (flags & TF_CLUIE_CURRENTPAGE) {
        safe_callback(on_candidate_list_change_page, candidates_.page);
    }

    return S_OK;
} BK_TRANSLATE_EXCEPTIONS_END

//------------------------------------------------------------------------------
//! Called when a UI element is closed.
//! @throw noexcept
//------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::EndUIElement(DWORD
    //dwUIElementId unused
) BK_TRANSLATE_EXCEPTIONS_BEGIN {
    BK_TRACE_BEGIN
    BK_TRACE_END

    candidates_ = candidate_list(); // clear the list.
    safe_callback(on_candidate_list_end);

    return S_OK;
} BK_TRANSLATE_EXCEPTIONS_END

////////////////////////////////////////////////////////////////////////////////
// ITfLanguageProfileNotifySink functions
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
//! Called when the input language is about to change.
//! @throw noexcept
//------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::OnLanguageChange(
    LANGID langid,
    __RPC__out BOOL *pfAccept
) BK_TRANSLATE_EXCEPTIONS_BEGIN {
    BK_TRACE_BEGIN
    BK_TRACE_END

    if (pfAccept == nullptr) {
        return E_INVALIDARG;
    }

    std::vector<wchar_t> buffer;
    auto const size = LCIDToLocaleName(langid, nullptr, 0, 0);
    buffer.resize(size);
    auto const result = LCIDToLocaleName(langid, buffer.data(), buffer.size(), 0);
    BK_THROW_ON_COND(::LCIDToLocaleName, result == 0);

    input_language_id_.assign(buffer.data(), buffer.data() + size);

    if (on_input_language_change) {
        on_input_language_change(
            converter_.to_bytes(input_language_id_)
        );
    }

    *pfAccept = true;

    return S_OK;
} BK_TRANSLATE_EXCEPTIONS_END

//------------------------------------------------------------------------------
//! Called when the input language has changed.
//! @throw noexcept
//------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::OnLanguageChanged()
BK_TRANSLATE_EXCEPTIONS_BEGIN {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return S_OK;
} BK_TRANSLATE_EXCEPTIONS_END

////////////////////////////////////////////////////////////////////////////////
// ITfContextOwnerCompositionSink
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
//! Called when a new input composition begins.
//! @throw noexcept
//------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::OnStartComposition(
    __RPC__in_opt ITfCompositionView* pComposition,
    __RPC__out BOOL* pfOk
) BK_TRANSLATE_EXCEPTIONS_BEGIN {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(pComposition)
        BK_TRACE_VAR(pfOk)
    BK_TRACE_END

    if (pComposition == nullptr || pfOk == nullptr) {
        return E_INVALIDARG;
    }

    *pfOk = TRUE;

    if (on_composition_begin) {
        on_composition_begin();
    }

    return S_OK;
} BK_TRANSLATE_EXCEPTIONS_END

namespace {
    ime::composition::attribute get_attribute(TF_DISPLAYATTRIBUTE const& a) {
        typedef ime::composition::attribute attr;

        switch (a.bAttr) {
        case TF_ATTR_INPUT :               return attr::input;
        case TF_ATTR_TARGET_CONVERTED :    return attr::target_converted;
        case TF_ATTR_CONVERTED :           return attr::converted;
        case TF_ATTR_TARGET_NOTCONVERTED : return attr::target_not_converted;
        case TF_ATTR_INPUT_ERROR :         return attr::input_error;
        case TF_ATTR_FIXEDCONVERTED :      return attr::fixed_conversion;
        case TF_ATTR_OTHER : default :     return attr::other;
        }
    }

    ime::composition::line_style get_line_style(TF_DISPLAYATTRIBUTE const& a) {
        typedef ime::composition::line_style ls;

        switch (a.lsStyle) {
        case TF_LS_NONE : default: return ls::none;
        case TF_LS_SOLID :         return ls::solid;
        case TF_LS_DOT :           return ls::dot;
        case TF_LS_DASH :          return ls::dash;
        case TF_LS_SQUIGGLE :      return ls::squiggle;
        }
    }
} // namespace

//------------------------------------------------------------------------------
//! Called when an input composition has been updated in any way.
//! @throw noexcept
//------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::OnUpdateComposition(
    __RPC__in_opt ITfCompositionView* pComposition,
    __RPC__in_opt ITfRange*           pRangeNew
) BK_TRANSLATE_EXCEPTIONS_BEGIN {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(pComposition)
        BK_TRACE_VAR(pRangeNew)
    BK_TRACE_END

    //! helper to hold information about a range of text
    struct composition_range_t {
        LONG start;
        LONG len;
        TF_DISPLAYATTRIBUTE attribute;
    };

    if (pComposition == nullptr) {
        return E_INVALIDARG;
    } else if (pRangeNew == nullptr) { // @todo why?
        return S_OK;
    }

    std::vector<composition_range_t> ranges;

    // properties to track.
    auto properties = win::make_com_ptr([&](ITfReadOnlyProperty** out) {
        GUID const* props[] = { &GUID_PROP_ATTRIBUTE, };

        auto context = win::make_com_ptr([&](ITfContext** out) {
            return pRangeNew->GetContext(out);
        });

        return context->TrackProperties(
            props, BK_ARRAY_ELEMENT_COUNT(props), nullptr, 0, out );
    });

    // enumeration of ranges in the active composition.
    auto range_enum = win::make_com_ptr([&](IEnumTfRanges** out) {
        return properties->EnumRanges(edit_cookie_, out, pRangeNew);
    });

    win::for_each_enum(range_enum, [&](ITfRange& range) {
        // we need start and end positions
        auto range_acp = win::make_com_ptr([&](ITfRangeACP** out) {
            return range.QueryInterface(out);
        });

        composition_range_t comp_info;
        range_acp->GetExtent(&comp_info.start, &comp_info.len);

        // enumeration of properties for this range.
        auto prop_enum = win::make_com_ptr([&](IEnumTfPropertyValue** out) {
            win::variant value;
            properties->GetValue(edit_cookie_, &range, value);
            return value->punkVal->QueryInterface(out);
        });

        win::for_each_enum(prop_enum, [&](TF_PROPERTYVAL& prop) {
            // only interested in attributes.
            if (!IsEqualGUID(prop.guidId, GUID_PROP_ATTRIBUTE) ||
                prop.varValue.vt != VT_I4
            ) {
                return;
            }

            // get the attribute.
            auto attribute = win::make_com_ptr([&](ITfDisplayAttributeInfo** out) {
                cat_manager_->GetGUID(prop.varValue.lVal, &prop.guidId);
                return attr_manager_->GetDisplayAttributeInfo(prop.guidId, out, nullptr);
            });

            attribute->GetAttributeInfo(&comp_info.attribute);
            ranges.emplace_back(std::move(comp_info));
        });
    });

    // build the range_list to pass to on_composition_update
    if (on_composition_update && !ranges.empty()) {
        ime::composition::range_list result;
        result.reserve(ranges.size());

        for (auto const& i : ranges) {
            ime::composition::range r = {
                converter_.to_bytes(text_.substr(i.start, i.len)),
                get_attribute(i.attribute),
                get_line_style(i.attribute)
            };

            result.emplace_back(std::move(r));
        }

        on_composition_update(std::move(result));
    }

    return S_OK;
} BK_TRANSLATE_EXCEPTIONS_END

//------------------------------------------------------------------------------
//! Called when an input composition has ended.
//! @throw noexcept
//------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::OnEndComposition(
    __RPC__in_opt ITfCompositionView *pComposition
) BK_TRANSLATE_EXCEPTIONS_BEGIN {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(pComposition)
    BK_TRACE_END

    if (pComposition == nullptr) {
        return E_INVALIDARG;
    }

    if (on_composition_end) {
        on_composition_end();
    }

    return S_OK;
} BK_TRANSLATE_EXCEPTIONS_END

////////////////////////////////////////////////////////////////////////////////
//ITfCompartmentEventSink functions
////////////////////////////////////////////////////////////////////////////////

namespace {
    //! Helper to convert from flags to ime::conversion_mode.
    ime::conversion_mode translate_conversion_mode(DWORD value, bool active) {
        if (value & TF_CONVERSIONMODE_NATIVE)       OutputDebugStringA("TF_CONVERSIONMODE_NATIVE\n");
        else                                        OutputDebugStringA("TF_CONVERSIONMODE_ALPHANUMERIC\n");     
        if (value & TF_CONVERSIONMODE_KATAKANA)     OutputDebugStringA("TF_CONVERSIONMODE_KATAKANA\n");
        if (value & TF_CONVERSIONMODE_FULLSHAPE)    OutputDebugStringA("TF_CONVERSIONMODE_FULLSHAPE\n");
        if (value & TF_CONVERSIONMODE_ROMAN)        OutputDebugStringA("TF_CONVERSIONMODE_ROMAN\n");
        if (value & TF_CONVERSIONMODE_CHARCODE)     OutputDebugStringA("TF_CONVERSIONMODE_CHARCODE\n");
        if (value & TF_CONVERSIONMODE_SOFTKEYBOARD) OutputDebugStringA("TF_CONVERSIONMODE_SOFTKEYBOARD\n");
        if (value & TF_CONVERSIONMODE_NOCONVERSION) OutputDebugStringA("TF_CONVERSIONMODE_NOCONVERSION\n");
        if (value & TF_CONVERSIONMODE_EUDC)         OutputDebugStringA("TF_CONVERSIONMODE_EUDC\n");
        if (value & TF_CONVERSIONMODE_SYMBOL)       OutputDebugStringA("TF_CONVERSIONMODE_SYMBOL\n");
        if (value & TF_CONVERSIONMODE_FIXED)        OutputDebugStringA("TF_CONVERSIONMODE_FIXED\n");

        if (!active) {
            return ime::conversion_mode::half_katakana;
        } else if (value & TF_CONVERSIONMODE_FULLSHAPE) {
            if (value & TF_CONVERSIONMODE_KATAKANA) {
                return ime::conversion_mode::full_katakana;
            } else if (value & TF_CONVERSIONMODE_NATIVE) {
                return ime::conversion_mode::full_hiragana;
            } else {
                return ime::conversion_mode::full_roman;
            }
        } else {
            if (value & TF_CONVERSIONMODE_KATAKANA) {
                return ime::conversion_mode::half_katakana;
            } else if (value & TF_CONVERSIONMODE_ROMAN) {
                return ime::conversion_mode::half_roman;
            } else {
                BK_TODO_BREAK; //! @todo handle this case
            }
        }
    }
}

//------------------------------------------------------------------------------
//! Called when a compartment value has changed (various input settings).
//! @throw noexcept
//------------------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::OnChange(
    __RPC__in REFGUID rguid
) BK_TRANSLATE_EXCEPTIONS_BEGIN {
    BK_TRACE_BEGIN
    BK_TRACE_END

    // local function to get the compartment value.
    auto get_value = [&] {
        VARIANT value;
        ::VariantInit(&value);

        // clean up.
        BK_ON_SCOPE_EXIT({
            ::VariantClear(&value);
        });

        auto compartment = win::make_com_ptr([&](ITfCompartment** out) {
            return compartment_manager_->GetCompartment(rguid, out);
        });

        auto const hr = compartment->GetValue(&value);
        BK_THROW_ON_FAIL(ITfCompartment::GetValue, hr);

        BK_ASSERT_MSG(value.vt == VT_I4, "expected an int");

        return value.intVal;
    };
 
    auto& status = conversion_status_;

    if (IsEqualGUID(rguid, GUID_COMPARTMENT_KEYBOARD_INPUTMODE_CONVERSION)) {
        status.conversion_mode = get_value();

        if (on_input_conversion_mode_change) {
            on_input_conversion_mode_change(
                translate_conversion_mode(status.conversion_mode, status.active == 1)
            );
        }
    } else if (IsEqualGUID(rguid, GUID_COMPARTMENT_KEYBOARD_INPUTMODE_SENTENCE)) {
        OutputDebugStringA("GUID_COMPARTMENT_KEYBOARD_INPUTMODE_SENTENCE\n");
        
        auto const value = get_value();
        conversion_status_.sentence_mode = value;

        if (value & TF_SENTENCEMODE_PLAURALCLAUSE) {
            OutputDebugStringA("TF_SENTENCEMODE_PLAURALCLAUSE\n");
        } else {
            OutputDebugStringA("TF_SENTENCEMODE_NONE\n");
        }
        if (value & TF_SENTENCEMODE_SINGLECONVERT) {
            OutputDebugStringA("TF_SENTENCEMODE_SINGLECONVERT\n");
        }
        if (value & TF_SENTENCEMODE_AUTOMATIC) {
            OutputDebugStringA("TF_SENTENCEMODE_AUTOMATIC\n");
        }
        if (value & TF_SENTENCEMODE_PHRASEPREDICT) {
            OutputDebugStringA("TF_SENTENCEMODE_PHRASEPREDICT\n");
        }
        if (value & TF_SENTENCEMODE_CONVERSATION) {
            OutputDebugStringA("TF_SENTENCEMODE_CONVERSATION\n");
        }
    } else if (IsEqualGUID(rguid, GUID_COMPARTMENT_KEYBOARD_OPENCLOSE)) {      
        status.active = get_value();

        if (on_input_activate) {
            on_input_activate(status.active == 1);
        }
    }

    return S_OK;
} BK_TRANSLATE_EXCEPTIONS_END

////////////////////////////////////////////////////////////////////////////////
// ITfThreadMgrEventSink
////////////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::OnInitDocumentMgr(
    __RPC__in_opt ITfDocumentMgr *pdim
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::OnUninitDocumentMgr(
    __RPC__in_opt ITfDocumentMgr *pdim
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::OnSetFocus(
    __RPC__in_opt ITfDocumentMgr *pdimFocus,
    __RPC__in_opt ITfDocumentMgr *pdimPrevFocus
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::OnPushContext(
    __RPC__in_opt ITfContext *pic
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::OnPopContext(
    __RPC__in_opt ITfContext *pic
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// ITfActiveLanguageProfileNotifySink
////////////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::OnActivated(
    __RPC__in REFCLSID clsid,
    __RPC__in REFGUID  guidProfile,
    BOOL fActivated
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return S_OK;
}

////////////////////////////////////////////////////////////////////////////////
// ITextStoreACP
////////////////////////////////////////////////////////////////////////////////
HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::AdviseSink(
    __RPC__in     REFIID    riid,
    __RPC__in_opt IUnknown* punk,
                  DWORD     dwMask
) {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(riid)
        BK_TRACE_VAR(punk)
        BK_TRACE_VAR(dwMask)
    BK_TRACE_END

    if (nullptr == punk) {
        return E_INVALIDARG;
    }

    win::com_ptr<IUnknown> key = win::make_com_ptr([&](IUnknown** out) {
        return punk->QueryInterface<IUnknown>(out);
    });

    //install the advise sink
    if (advise_sink_.key == nullptr) {
        if (!IsEqualIID(riid, __uuidof(ITextStoreACPSink))) {
            return E_INVALIDARG;
        }

        advise_sink_.key   = key;
        advise_sink_.flags = dwMask;
        advise_sink_.sink  = win::make_com_ptr([&](ITextStoreACPSink** out) {
            return punk->QueryInterface<ITextStoreACPSink>(out);
        });
    //update the flags
    } else if (advise_sink_.key == key) {
        advise_sink_.flags = dwMask;
    //otherwise, ignore
    } else {
        return CONNECT_E_ADVISELIMIT;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::UnadviseSink(
    __RPC__in_opt IUnknown* punk
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::RequestLock(
               DWORD    dwLockFlags,
    __RPC__out HRESULT* phrSession
) {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(dwLockFlags)
        BK_TRACE_VAR(phrSession)
    BK_TRACE_END

    if (phrSession == nullptr) {
        return E_INVALIDARG;
    } else if (advise_sink_.sink == nullptr) {
        return E_UNEXPECTED;
    }

    bool const want_sync  = (dwLockFlags & TS_LF_SYNC) != 0;
    //bool const want_read  = (dwLockFlags & TS_LF_READ) != 0;
    //bool const want_write = (dwLockFlags & TS_LF_READWRITE) != 0;

    if (lock_.is_locked()) {
        return E_NOTIMPL;
    }

    HRESULT hr = S_OK;

    if (want_sync) {
        lock_.lock(dwLockFlags);
        hr = advise_sink_.sink->OnLockGranted(dwLockFlags);
        lock_.unlock();
    }

    return hr;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::GetStatus(
    __RPC__out TS_STATUS* pdcs
) {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(pdcs)
    BK_TRACE_END

    if (nullptr == pdcs) {
        return E_INVALIDARG;
    }

    pdcs->dwDynamicFlags = 0;
    pdcs->dwStaticFlags  = TS_SS_NOHIDDENTEXT;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::QueryInsert(
               LONG  acpTestStart,
               LONG  acpTestEnd,
               ULONG cch,
    __RPC__out LONG* pacpResultStart,
    __RPC__out LONG* pacpResultEnd
) {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(acpTestStart)
        BK_TRACE_VAR(acpTestEnd)
        BK_TRACE_VAR(cch)
        BK_TRACE_VAR(pacpResultStart)
        BK_TRACE_VAR(pacpResultEnd)
    BK_TRACE_END

    if (pacpResultStart == nullptr || pacpResultEnd == nullptr) {
        return E_INVALIDARG;
    }

    auto const length = static_cast<LONG>(text_.length());

    //The value -1 signals the end of the buffer
    acpTestEnd = (acpTestEnd == -1) ? length : acpTestEnd;

    //Basic sanity checks
    if (acpTestStart > length ||
        acpTestStart < 0      ||
        acpTestEnd   > length ||
        acpTestEnd   < acpTestStart
    ) {
        return E_INVALIDARG;
    }

    ULONG const sel_length = acpTestEnd - acpTestStart;

    //insertion point
    if (sel_length == 0) {
        *pacpResultStart = acpTestStart + cch;
        *pacpResultEnd   = acpTestStart + cch;
    //substitution -> highlight
    } else if (sel_length == cch) {
        *pacpResultStart = acpTestStart;
        *pacpResultEnd   = acpTestEnd;
    //replacement
    } else {
        *pacpResultStart = acpTestStart;
        *pacpResultEnd   = acpTestStart + cch;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::GetSelection(
    ULONG ulIndex,
    ULONG ulCount,
    __RPC__out_ecount_part(ulCount, *pcFetched) TS_SELECTION_ACP* pSelection,
    __RPC__out ULONG* pcFetched
) {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(ulIndex)
        BK_TRACE_VAR(ulCount)
        //BK_TRACE_VAR(pSelection)
        //BK_TRACE_VAR(pcFetched)
    BK_TRACE_END

    if (pSelection == nullptr || pcFetched == nullptr) {
        return E_INVALIDARG;
    } else if (!lock_.is_read()) {
        return TS_E_NOLOCK;
    }

    if (ulIndex != TF_DEFAULT_SELECTION) {
        return E_NOTIMPL;
    } else if (ulCount != 1) {
        return E_NOTIMPL;
    }

    pSelection[0] = selection_;
    *pcFetched    = 1;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::SetSelection(
    ULONG ulCount,
    __RPC__in_ecount_full(ulCount) const TS_SELECTION_ACP* pSelection
) {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(ulCount)
        BK_TRACE_VAR(pSelection)
    BK_TRACE_END

    if (nullptr == pSelection) {
        return E_INVALIDARG;
    } else if (!lock_.is_write()) {
        return TS_E_NOLOCK;
    }

    if (ulCount != 1) {
        return E_NOTIMPL;
    }

    auto const length = static_cast<LONG>(text_.length());
    auto const start  = pSelection->acpStart;
    auto const end    = pSelection->acpEnd == -1 ? length : pSelection->acpEnd;

    if (start < 0 || end < start || end > length) {
        return TS_E_INVALIDPOS;
    }

    selection_.acpStart = start;
    selection_.acpEnd   = end;
    selection_.style    = pSelection->style;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::GetText(
    LONG acpStart,
    LONG acpEnd,
    __RPC__out_ecount_part(cchPlainReq, *pcchPlainRet) WCHAR* pchPlain,
    ULONG cchPlainReq,
    __RPC__out ULONG* pcchPlainRet,
    __RPC__out_ecount_part(cRunInfoReq, *pcRunInfoRet) TS_RUNINFO* prgRunInfo,
    ULONG cRunInfoReq,
    __RPC__out ULONG* pcRunInfoRet,
    __RPC__out LONG* pacpNext
) {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(acpStart)
        BK_TRACE_VAR(acpEnd)
        BK_TRACE_VAR(pchPlain)
        BK_TRACE_VAR(cchPlainReq)
        BK_TRACE_VAR(pcchPlainRet)
        BK_TRACE_VAR(prgRunInfo)
        BK_TRACE_VAR(cRunInfoReq)
        BK_TRACE_VAR(pcRunInfoRet)
        BK_TRACE_VAR(pacpNext)
    BK_TRACE_END

    if (nullptr == pchPlain && cchPlainReq != 0) {
        return E_INVALIDARG;
    } else if (nullptr == prgRunInfo && cRunInfoReq != 0) {
        return E_INVALIDARG;
    } else if (pcRunInfoRet == nullptr || pacpNext == nullptr || pcchPlainRet == nullptr) {
        return E_INVALIDARG;
    }

    if (!lock_.is_read()) {
        return TF_E_NOLOCK;
    }

    auto const text_length = static_cast<LONG>(text_.length());

    if (acpStart < 0 || acpEnd > text_length) {
        return E_INVALIDARG;
    }

    unsigned const start            = acpStart;
    unsigned const end              = (acpEnd == -1) ? text_length : acpEnd;
    unsigned const requested_length = end - start;

    bool const want_text = pchPlain != nullptr;
    bool const want_info = prgRunInfo != nullptr;

    if (want_text) {
        if (requested_length <= cchPlainReq) {
            std::copy(text_.cbegin() + start, text_.cbegin() + end, pchPlain);

            *pcchPlainRet = requested_length;
            *pacpNext     = end + 1;
        } else {
            BK_TODO_BREAK;
            return E_NOTIMPL; //TODO
        }
    }

    if (want_info) {
        prgRunInfo->type   = TS_RT_PLAIN;
        prgRunInfo->uCount = text_length;

        *pcRunInfoRet = 1;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::SetText(
    DWORD dwFlags,
    LONG acpStart,
    LONG acpEnd,
    __RPC__in_ecount_full(cch) const WCHAR *pchText,
    ULONG cch,
    __RPC__out TS_TEXTCHANGE *pChange
) {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(dwFlags)
        BK_TRACE_VAR(acpStart)
        BK_TRACE_VAR(acpEnd)
        BK_TRACE_VAR(pchText)
        BK_TRACE_VAR(cch)
        BK_TRACE_VAR(pChange)
    BK_TRACE_END

    if (pchText == nullptr || pChange == nullptr) {
        return E_INVALIDARG;
    } else if (!lock_.is_write()) {
        return TS_E_NOLOCK;
    }

    if (dwFlags == TS_ST_CORRECTION) {
        return E_NOTIMPL;
    }

    TS_SELECTION_ACP selection;
    selection.acpStart = acpStart;
    selection.acpEnd   = acpEnd;
    selection.style.fInterimChar = FALSE;
    selection.style.ase = TS_AE_END;

    if (FAILED(SetSelection(1, &selection))) {
        return TF_E_INVALIDPOS;
    }

    if (FAILED(InsertTextAtSelection(TS_IAS_NOQUERY, pchText, cch, nullptr, nullptr, pChange))) {
        return E_FAIL;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::GetFormattedText(
    LONG acpStart,
    LONG acpEnd,
    __RPC__deref_out_opt IDataObject **ppDataObject
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::GetEmbedded(
    LONG acpPos,
    __RPC__in REFGUID rguidService,
    __RPC__in REFIID riid,
    __RPC__deref_out_opt IUnknown **ppunk
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::QueryInsertEmbedded(
    __RPC__in const GUID *pguidService,
    __RPC__in const FORMATETC *pFormatEtc,
    __RPC__out BOOL *pfInsertable
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::InsertEmbedded(
    DWORD dwFlags,
    LONG acpStart,
    LONG acpEnd,
    __RPC__in_opt IDataObject *pDataObject,
    __RPC__out TS_TEXTCHANGE *pChange
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::InsertTextAtSelection(
    DWORD dwFlags,
    __RPC__in_ecount_full(cch) const WCHAR* pchText,
    ULONG cch,
    __RPC__out LONG* pacpStart,
    __RPC__out LONG* pacpEnd,
    __RPC__out TS_TEXTCHANGE* pChange
) {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(dwFlags)
        BK_TRACE_VAR(pchText)
        BK_TRACE_VAR(cch)
        BK_TRACE_VAR(pacpStart)
        BK_TRACE_VAR(pacpEnd)
        BK_TRACE_VAR(pChange)
    BK_TRACE_END

    if (pchText == nullptr && cch != 0) {
        return E_INVALIDARG;
    } else if (!lock_.is_write()) {
        return TS_E_NOLOCK;
    }

    //Query only -- do not do insertion
    if (TS_IAS_QUERYONLY == dwFlags) {
        HRESULT hr = QueryInsert(selection_.acpStart, selection_.acpEnd, cch, pacpStart, pacpEnd);
        if (FAILED(hr)) {
            return hr;
        }
    //Insert, no query
    } else if (TS_IAS_NOQUERY == dwFlags) {
        if (pChange == nullptr) {
            return E_INVALIDARG;
        }

        text_.replace(
            text_.begin() + selection_.acpStart,
            text_.begin() + selection_.acpEnd,
            pchText,
            cch
        );

        pChange->acpStart  = selection_.acpStart;
        pChange->acpOldEnd = selection_.acpEnd;

        if (selection_.length() == 0) {
            selection_.acpStart += cch;
            selection_.acpEnd   += cch;
        } else {
            selection_.acpEnd = selection_.acpStart + cch;
        }

        pChange->acpNewEnd = selection_.acpEnd;
    } else if (0 == dwFlags) {
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::InsertEmbeddedAtSelection(
    DWORD dwFlags,
    __RPC__in_opt IDataObject *pDataObject,
    __RPC__out LONG *pacpStart,
    __RPC__out LONG *pacpEnd,
    __RPC__out TS_TEXTCHANGE *pChange
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::RequestSupportedAttrs(
    DWORD dwFlags,
    ULONG cFilterAttrs,
    __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID* paFilterAttrs
) {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(dwFlags)
        BK_TRACE_VAR(cFilterAttrs)
    BK_TRACE_END

    if (dwFlags == TS_ATTR_FIND_WANT_VALUE) {
        return E_NOTIMPL;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::RequestAttrsAtPosition(
    LONG acpPos,
    ULONG cFilterAttrs,
    __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID *paFilterAttrs,
    DWORD dwFlags
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::RequestAttrsTransitioningAtPosition(
    LONG acpPos,
    ULONG cFilterAttrs,
    __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID *paFilterAttrs,
    DWORD dwFlags
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::FindNextAttrTransition(
    LONG acpStart,
    LONG acpHalt,
    ULONG cFilterAttrs,
    __RPC__in_ecount_full_opt(cFilterAttrs) const TS_ATTRID *paFilterAttrs,
    DWORD dwFlags,
    __RPC__out LONG *pacpNext,
    __RPC__out BOOL *pfFound,
    __RPC__out LONG *plFoundOffset
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::RetrieveRequestedAttrs(
    ULONG ulCount,
    __RPC__out_ecount_part(ulCount, *pcFetched) TS_ATTRVAL *paAttrVals,
    __RPC__out ULONG *pcFetched
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    if (paAttrVals->varValue.vt != VT_EMPTY) {
        return E_FAIL;
    }

    *pcFetched = 1;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::GetEndACP(
    __RPC__out LONG *pacp
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::GetActiveView(
    __RPC__out TsViewCookie *pvcView
) {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(pvcView)
    BK_TRACE_END

    if (nullptr == pvcView) {
        return E_INVALIDARG;
    }

    *pvcView = view_cookie_;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::GetACPFromPoint(
    TsViewCookie vcView,
    __RPC__in const POINT *ptScreen,
    DWORD dwFlags,
    __RPC__out LONG *pacp
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::GetTextExt(
    TsViewCookie vcView,
    LONG acpStart,
    LONG acpEnd,
    __RPC__out RECT *prc,
    __RPC__out BOOL *pfClipped
) BK_TRANSLATE_EXCEPTIONS_BEGIN {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(vcView)
        BK_TRACE_VAR(acpStart)
        BK_TRACE_VAR(acpEnd)
        BK_TRACE_VAR(prc)
        BK_TRACE_VAR(pfClipped)
    BK_TRACE_END

    if (nullptr == prc || nullptr == pfClipped) {
        return E_INVALIDARG;
    }

    prc->left = 0;
    prc->top  = 0;

    prc->right   = 100;
    prc->bottom  = 1000;

    *pfClipped = FALSE;

    return S_OK;
} BK_TRANSLATE_EXCEPTIONS_END

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::GetScreenExt(
    TsViewCookie vcView,
    __RPC__out RECT *prc
) {
    BK_TRACE_BEGIN
    BK_TRACE_END

    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE
impl::ime_manager_impl_t::GetWnd(
    TsViewCookie vcView,
    __RPC__deref_out_opt HWND *phwnd
) BK_TRANSLATE_EXCEPTIONS_BEGIN {
    BK_TRACE_BEGIN
        BK_TRACE_VAR(vcView)
        BK_TRACE_VAR(phwnd)
    BK_TRACE_END

    if (nullptr == phwnd) {
        return E_INVALIDARG;
    }

    *phwnd = window_;

    return S_OK;
} BK_TRANSLATE_EXCEPTIONS_END

#undef BK_TRANSLATE_EXCEPTIONS_BEGIN
#undef BK_TRANSLATE_EXCEPTIONS_END

#pragma warning( default : 4100 )
#pragma warning( default : 4702 )
