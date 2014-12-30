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
#ifndef WIN32_RIGHTS_HPP_INCLUDED
#define WIN32_RIGHTS_HPP_INCLUDED

#include <cstdint>

#include <windows.h>

namespace win32 {

using namespace std;

typedef uint32_t access_mask;

enum class common_access_rights : access_mask {
    delete_object       = DELETE,
    read_control        = READ_CONTROL,
    write_dac           = WRITE_DAC,
    write_owner         = WRITE_OWNER,
    synchronize         = SYNCHRONIZE,
    system_security     = ACCESS_SYSTEM_SECURITY,
    read_write_execute  = GENERIC_ALL,
    execute             = GENERIC_EXECUTE,
    write               = GENERIC_WRITE,
    read                = GENERIC_READ
};

} // namespace win32

#endif // WIN32_RIGHTS_HPP_INCLUDED
