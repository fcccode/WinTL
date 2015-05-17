////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2015 Putta Khunchalee
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
////////////////////////////////////////////////////////////////////////////////
#ifndef WIN32_EVENT_TRACING_HPP_INCLUDED
#define WIN32_EVENT_TRACING_HPP_INCLUDED

#include "guid.hpp"

#include <functional>
#include <system_error>
#include <stdexcept>
#include <string>
#include <initializer_list>
#include <limits>

#include <cstring>
#include <cinttypes>
#include <cstddef>

#include <windows.h>

#include <evntrace.h>
#include <evntprov.h>

#include <malloc.h>

namespace win32 {

class manifest_event_data final {
public:
    manifest_event_data(const char *s)
    {
        auto len = std::strlen(s) + 1;

#pragma push_macro("max")
#undef max
        if (len > std::numeric_limits<std::uint32_t>::max()) {
            throw std::length_error("Length of string is out of limit.");
        }
#pragma pop_macro("max")

        addr_ = s;
        len_ = static_cast<std::uint32_t>(len);
    }

    manifest_event_data(const std::string& s) : manifest_event_data(s.c_str())
    {
    }

    manifest_event_data(const wchar_t *s)
    {
        auto len = (std::wcslen(s) + 1) * sizeof(wchar_t);

#pragma push_macro("max")
#undef max
        if (len > std::numeric_limits<std::uint32_t>::max()) {
            throw std::length_error("Length of string is out of limit.");
        }
#pragma pop_macro("max")

        addr_ = s;
        len_ = static_cast<std::uint32_t>(len);
    }

    manifest_event_data(const std::wstring& s) : manifest_event_data(s.c_str())
    {
    }

    template<typename T>
    manifest_event_data(const T& f) : addr_(&f), len_(sizeof(f))
    {
    }

    const void * address() const
    {
        return addr_;
    }

    std::uint32_t length() const
    {
        return len_;
    }
private:
    const void *addr_;
    std::uint32_t len_;
};

class manifest_event_provider final {
public:
    enum class mode : ULONG {
        disable         = EVENT_CONTROL_CODE_DISABLE_PROVIDER,  // 0
        enable          = EVENT_CONTROL_CODE_ENABLE_PROVIDER,   // 1
        capture_state   = EVENT_CONTROL_CODE_CAPTURE_STATE,     // 2
    };

    enum class filter_type : std::uint32_t {
        none            = EVENT_FILTER_TYPE_NONE,               // 0x00000000
        schematized     = EVENT_FILTER_TYPE_SCHEMATIZED,        // 0x80000000
        system_flags    = EVENT_FILTER_TYPE_SYSTEM_FLAGS,       // 0x80000001
        tracehandle     = EVENT_FILTER_TYPE_TRACEHANDLE,        // 0x80000002
        pid             = EVENT_FILTER_TYPE_PID,                // 0x80000004
        executable_name = EVENT_FILTER_TYPE_EXECUTABLE_NAME,    // 0x80000008
        payload         = EVENT_FILTER_TYPE_PAYLOAD,            // 0x80000100
        event_id        = EVENT_FILTER_TYPE_EVENT_ID,           // 0x80000200
        stackwalk       = EVENT_FILTER_TYPE_STACKWALK,          // 0x80001000
    };

    struct filter {
        std::uint64_t data;
        std::uint32_t size;
        filter_type type;
    };

    typedef std::function<void(
            manifest_event_provider&,
            const guid&,
            mode,
            std::uint8_t,
            std::uint64_t,
            std::uint64_t,
            const filter&)> enable_callback;

    manifest_event_provider(
            const guid& id,
            const enable_callback enable_cb = nullptr) :
                id_(id),
                enable_cb_(enable_cb),
                m_(mode::disable)
    {
        ULONG res;

        if (enable_cb_) {
            res = EventRegister(id_, on_enable_trunk, this, &h_);
        } else {
            res = EventRegister(id_, nullptr, nullptr, &h_);
        }

        if (res != ERROR_SUCCESS) {
            throw std::system_error(res, std::system_category());
        }
    }

    manifest_event_provider(const manifest_event_provider&) = delete;

    ~manifest_event_provider()
    {
        auto res = EventUnregister(h_);
        if (res != ERROR_SUCCESS) {
            throw std::system_error(res, std::system_category());
        }
    }

    manifest_event_provider& operator = (
            const manifest_event_provider&) = delete;

    void write(const EVENT_DESCRIPTOR& evt)
    {
        auto res = EventWrite(h_, &evt, 0, nullptr);
        if (res != ERROR_SUCCESS) {
            throw std::system_error(res, std::system_category());
        }
    }

    void write(
            const EVENT_DESCRIPTOR& evt,
            std::initializer_list<manifest_event_data> data)
    {
        EVENT_DATA_DESCRIPTOR *dc;
        std::size_t i;

        dc = reinterpret_cast<EVENT_DATA_DESCRIPTOR *>(_malloca(
                data.size() * sizeof(EVENT_DATA_DESCRIPTOR)));
        i = 0;

        try {
            for (const auto& d : data) {
                EventDataDescCreate(&dc[i++], d.address(), d.length());
            }
        } catch (...) {
            _freea(dc);
            throw;
        }

        auto res = EventWrite(h_, &evt, static_cast<ULONG>(data.size()), dc);
        _freea(dc);

        if (res != ERROR_SUCCESS) {
            throw std::system_error(res, std::system_category());
        }
    }
private:
    guid id_;
    REGHANDLE h_;
    enable_callback enable_cb_;
    mode m_;

    void on_enable(
            const guid& sid,
            mode m,
            std::uint8_t l,
            std::uint64_t kmask,
            std::uint64_t kbits,
            const filter& f)
    {
        enable_cb_(*this, sid, m, l, kmask, kbits, f);
    }

    static void NTAPI on_enable_trunk(
            LPCGUID sid,
            ULONG m,
            UCHAR l,
            ULONGLONG kmask,
            ULONGLONG kbits,
            PEVENT_FILTER_DESCRIPTOR f,
            PVOID ctx)
    {
        reinterpret_cast<manifest_event_provider *>(ctx)->on_enable(
                *sid,
                static_cast<mode>(m),
                l,
                kmask,
                kbits,
                *reinterpret_cast<filter *>(f));
    }
};

} // namespace win32

#endif // WIN32_EVENT_TRACING_HPP_INCLUDED
