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
#ifndef WIN32_FILE_HPP_INCLUDED
#define WIN32_FILE_HPP_INCLUDED

#include <win32/rights.hpp>

#include <windows.h>

namespace win32 {

enum class file_access_rights : access_mask {
    list_directory              = FILE_LIST_DIRECTORY,          // 1
    read_data                   = FILE_READ_DATA,               // 1
    add_file                    = FILE_ADD_FILE,                // 2
    write_data                  = FILE_WRITE_DATA,              // 2
    add_subdir                  = FILE_ADD_SUBDIRECTORY,        // 4
    append_data                 = FILE_APPEND_DATA,             // 4
    create_pipe_instance        = FILE_CREATE_PIPE_INSTANCE,    // 4
    read_extended_attributes    = FILE_READ_EA,                 // 8
    write_extended_attributes   = FILE_WRITE_EA,                // 16
    execute                     = FILE_EXECUTE,                 // 32
    traverse_directory          = FILE_TRAVERSE,                // 32
    delete_child                = FILE_DELETE_CHILD,            // 64
    read_attributes             = FILE_READ_ATTRIBUTES,         // 128
    write_attributes            = FILE_WRITE_ATTRIBUTES,        // 256
    read                        = FILE_GENERIC_READ,
    write                       = FILE_GENERIC_WRITE,
    all                         = FILE_ALL_ACCESS
};

enum class file_share_modes : DWORD {
    none        = 0,
    read_data   = FILE_SHARE_READ,
    write_data  = FILE_SHARE_WRITE,
    delete_file = FILE_SHARE_DELETE
};

} // namespace win32

#endif // WIN32_FILE_HPP_INCLUDED
