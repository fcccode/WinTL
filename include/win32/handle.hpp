////////////////////////////////////////////////////////////////////////////////
// Copyright (C) 2014 Putta Khunchalee
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
#ifndef WIN32_HANDLE_HPP_INCLUDED
#define WIN32_HANDLE_HPP_INCLUDED

#include <win32/object.hpp>

#include <utility>

#include <windows.h>

namespace win32 {

class handle final {
public:
    handle(HANDLE h, HANDLE invalid) :
        data_(new handle_data(h, invalid)) = default
    {
    }

    handle(handle&& src) : data_(std::move(src.data_))
    {
    }

    operator HANDLE() const
    {
        return data_->handle();
    }

    handle& operator=(handle&& src)
    {
        data_ = std::move(src.data_);
        return *this;
    }
protected:
    class handle_data : public refcounting {
    public:
        handle_data(HANDLE h, HANDLE invalid) : h_(h), invalid_handle_(invalid)
        {
        }

        handle_data(const handle_data&) = delete;
        handle_data& operator=(const handle_data&) = delete;

        HANDLE handle() const
        {
            return h_;
        }
    protected:
        virtual ~handle_data()
        {
            if (h_ != invalid_handle_) ::CloseHandle(h_);
        }
    private:
        HANDLE h_;
        HANDLE invalid_handle_;
    };
private:
    ref<handle_data> data_;
};

} // namespace win32

#endif // WIN32_HANDLE_HPP_INCLUDED
