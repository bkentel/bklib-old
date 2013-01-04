#pragma once

#include <chrono>
#include <initializer_list>

namespace bklib {

#define BK_MAKE_SOURCE_INFO() \
    ::bklib::source_info(__FILEW__, __FUNCTION__, __LINE__)

#define BK_LOG_MESSAGE(LOG, MSG) \
    LOG.write(BK_MAKE_SOURCE_INFO(), MSG)

//stores source code information
struct source_info {
    source_info(wchar_t const* file, char const* function, unsigned line)
        : file(file), function(function), line(line) { }

    wchar_t const* file;
    char const*    function;
    unsigned       line;
};

//stores source code info and a timestamp for when constructed
struct log_record {
    log_record(source_info&& info)
        : src_info(info)
        , time_point(std::chrono::system_clock::now())
    {
    }

    source_info src_info;
    std::chrono::time_point<std::chrono::system_clock> time_point;
};

//formatted message in utf-8
struct formatted_record {
    formatted_record(log_record&& record, utf8string&& message)
        : record(record), message(message)
    {
    }

    log_record record;
    utf8string message;
};

struct default_sink {
    void write(formatted_record&& record) const {
        OutputDebugStringA(record.message.c_str());
    }
};

//filters nothing
struct default_filter {
    bool test(log_record const& record) const {
        return true;
    }
};

//does nothing
struct default_format {
    formatted_record apply(log_record&& record, utf8string&& message) const {
        return formatted_record(std::move(record), std::move(message));
    }
};

template <
    typename sink_t   = default_sink,
    typename filter_t = default_filter,
    typename format_t = default_format
>
struct log_t {
    log_t() { }

    template <typename T>
    void write(log_record&& record, T&& msg) {
        if (filter.test(record)) {
            sink.write(
                format.apply(
                    std::move(record),
                    std::forward<T>(msg)
                )
            );
        }
    }

    sink_t   sink;
    filter_t filter;
    format_t format;
};


} //namespace bklib
