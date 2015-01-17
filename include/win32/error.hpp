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
#ifndef WIN32_ERROR_HPP_INCLUDED
#define WIN32_ERROR_HPP_INCLUDED

#include <sstream>
#include <string>

#include <cstdarg>

#include <windows.h>

namespace win32 {

inline std::wstring format_error_message(unsigned long code, std::va_list args)
{
    LPWSTR buf;

    auto res = ::FormatMessageW(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_MAX_WIDTH_MASK,
            nullptr, code, 0, reinterpret_cast<LPWSTR>(&buf), 128, &args);

    if (!res) {
        std::wostringstream fmt(L"Unknown Error");
        fmt << L" (" << code << L")";
        return fmt.str();
    }

    std::wstring msg(buf);
    ::LocalFree(buf);

    return msg;
}

inline std::wstring format_error_message(unsigned long code, ...)
{
    std::va_list args;

    va_start(args, code);
    auto msg = format_error_message(code, args);
    va_end(args);

    return msg;
}

} // namespace win32

#endif // WIN32_ERROR_HPP_INCLUDED
