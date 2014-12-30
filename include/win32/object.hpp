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
#ifndef WIN32_OBJECT_HPP_INCLUDED
#define WIN32_OBJECT_HPP_INCLUDED

#include <atomic>

namespace win32 {

using namespace std;

class refcounting {
public:
    refcounting(const refcounting&) = delete;
    refcounting& operator=(const refcounting&) = delete;

    unsigned long decref()
    {
        auto refcnt = refcnt_.fetch_sub(1) - 1;
        if (refcnt == 0) delete this;
        return refcnt;
    }

    unsigned long incref()
    {
        return refcnt_.fetch_add(1) + 1;
    }
protected:
    refcounting(unsigned long initial = 1) : refcnt_(initial) = default
    {
    }

    virtual ~refcounting()
    {
    }
private:
    atomic_ulong refcnt_;
};

template<class T>
class ref final {
public:
    template<class Y>
    explicit ref(Y *ptr = nullptr) : ptr_(ptr) = default
    {
    }

    template<class Y>
    ref(const ref<Y>& that) : ptr_(that.ptr_)
    {
        if (ptr_) ptr_->incref();
    }

    ~ref()
    {
        if (ptr_) ptr_->decref();
    }

    T * operator->() const
    {
        return ptr_;
    }

    template<class Y>
    ref& operator=(const ref<Y>& that)
    {
        if (ptr_) ptr_->decref();
        ptr_ = that.ptr_;
        if (ptr_) ptr_->incref();
        return *this;
    }
private:
    T *ptr_;
};

} // namespace win32

#endif // WIN32_OBJECT_HPP_INCLUDED
