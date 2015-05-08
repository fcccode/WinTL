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
#ifndef WIN32_COM_HPP_INCLUDED
#define WIN32_COM_HPP_INCLUDED

#include <system_error>

#include <windows.h>
#include <objbase.h>

namespace win32 {

enum class com_threading_model {
    apartment,
    free,
    both,
    neutral
};

class com_context final {
public:
    enum class flags : DWORD {
        multi_threaded      = COINIT_MULTITHREADED,     // 0x00
        apartment_threaded  = COINIT_APARTMENTTHREADED, // 0x02
        disable_ole1dde     = COINIT_DISABLE_OLE1DDE,   // 0x04
        speed_over_memory   = COINIT_SPEED_OVER_MEMORY, // 0x08
    };

    com_context(flags f)
    {
        auto r = CoInitializeEx(nullptr, static_cast<DWORD>(f));
        if (FAILED(r)) {
            throw std::system_error(
                    r,
                    std::system_category(),
                    "CoInitializeEx() failed");
        }
    }

    com_context(const com_context&) = delete;

    ~com_context()
    {
        CoUninitialize();
    }

    com_context& operator = (const com_context&) = delete;
};

} // namespace win32

#endif // WIN32_COM_HPP_INCLUDED
