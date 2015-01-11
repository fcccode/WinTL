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
#ifndef WIN32_SECURITY_HPP_INCLUDED
#define WIN32_SECURITY_HPP_INCLUDED

#include <system_error>

#include <cstring>

#include <windows.h>

namespace win32 {

class security_descriptor final {
public:
    security_descriptor() = default
    {
        data_ = reinterpret_cast<PSECURITY_DESCRIPTOR>(
                ::LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH));

        if (!data_) {
            auto err = ::GetLastError();
            throw std::system_error(err, std::system_category());
        }

        try {
            if (!InitializeSecurityDescriptor(data_,
                    SECURITY_DESCRIPTOR_REVISION)) {
                auto err = ::GetLastError();
                throw std::system_error(err, std::system_category());
            }
        } catch (...) {
            ::LocalFree(data_);
            throw;
        }
    }

    explicit security_descriptor(PSECURITY_DESCRIPTOR data) : data_(data)
    {
    }

    security_descriptor(security_descriptor&& src) : data_(src.data_)
    {
        src.data_ = nullptr;
    }

    security_descriptor(const security_descriptor&) = delete;

    ~security_descriptor()
    {
        if (data_) ::LocalFree(data_);
    }

    operator PSECURITY_DESCRIPTOR()
    {
        return data_;
    }

    security_descriptor& operator=(security_descriptor&& src)
    {
        if (data_) ::LocalFree(data_);

        data_ = src.data_;
        src.data_ = nullptr;

        return *this;
    }

    security_descriptor& operator=(const security_descriptor&) = delete;
private:
    PSECURITY_DESCRIPTOR data_;
};

class security_attributes final {
public:
    explicit security_attributes(security_descriptor *sd = nullptr,
            bool inherit_handle = false) = default : sd_(sd)
    {
        init(inherit_handle);
    }

    security_attributes(security_attributes&& src) : data_(src.data_),
            sd_(src.sd_)
    {
        src.data_.lpSecurityDescriptor = nullptr;
        src.sd_ = nullptr;
    }

    security_attributes(const security_attributes&) = delete;
    security_attributes& operator=(const security_attributes&) = delete;

    operator LPSECURITY_ATTRIBUTES()
    {
        return &data_;
    }
private:
    SECURITY_ATTRIBUTES data_;
    security_descriptor *sd_;

    void init(bool inherit_handle = false)
    {
        std::memset(&data_, 0, sizeof(data_));
        data_.nLength = sizeof(data_);
        data_.lpSecurityDescriptor = sd_ ? *sd_ : nullptr;
        data_.bInheritHandle = inherit_handle ? TRUE : FALSE;
    }
};

} // namespace win32

#endif // WIN32_SECURITY_HPP_INCLUDED
