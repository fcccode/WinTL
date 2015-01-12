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
#ifndef WIN32_SERVICE_HPP_INCLUDED
#define WIN32_SERVICE_HPP_INCLUDED

#include <vector>
#include <system_error>
#include <functional>
#include <string>

#include <cinttypes>
#include <cstring>

#include <windows.h>

namespace win32 {

#ifdef _M_AMD64
static const std::uint8_t service_main_trunk[] = {
    0x48, 0x83, 0xEC, 0x18, 0x4C, 0x8B, 0xC2, 0x8B,
    0xD1, 0x48, 0xB9, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0x48, 0xB8, 0xFF, 0xFF, 0xFF,
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xD0, 0x48,
    0x83, 0xC4, 0x18, 0xC3
};

#define SERVICE_MAIN_TRUNK_THIS_OFFSET 11
#define SERVICE_MAIN_TRUNK_PROC_OFFSET 21
#else
#error target platform is not supported.
#endif

class service final {
public:
    service(const std::wstring& name,
            const std::function<void(int, wchar_t **)>& proc) : name_(name),
            proc_(proc)
    {
        proctrunk_ = reinterpret_cast<std::uint8_t *>(::VirtualAlloc(nullptr,
                sizeof(service_main_trunk), MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE));
        if (!proctrunk_) {
            auto err = ::GetLastError();
            throw std::system_error(err, std::system_category());
        }

        std::memcpy(proctrunk_, service_main_trunk, sizeof(service_main_trunk));

        reinterpret_cast<std::intptr_t *>(proctrunk_ +
                SERVICE_MAIN_TRUNK_THIS_OFFSET)[0] =
                reinterpret_cast<std::intptr_t>(this);

        union {
            VOID (WINAPI service::*x)(DWORD, LPWSTR *);
            std::intptr_t addr;
        } svcmain = {&service::service_boot};

        reinterpret_cast<std::intptr_t *>(proctrunk_ +
                SERVICE_MAIN_TRUNK_PROC_OFFSET)[0] = svcmain.addr;
    }

    service(const service&) = delete;

    ~service()
    {
        if (proctrunk_) ::VirtualFree(proctrunk_, 0, MEM_RELEASE);
    }

    service& operator=(const service&) = delete;

    LPSERVICE_MAIN_FUNCTIONW bootstrapper() const
    {
        return reinterpret_cast<LPSERVICE_MAIN_FUNCTIONW>(proctrunk_);
    }

    const std::wstring& name() const
    {
        return name_;
    }
private:
    std::wstring name_;
    std::function<void(int, wchar_t **)> proc_;
    std::uint8_t *proctrunk_;

    void service_boot(DWORD dwArgc, LPWSTR *lpszArgv)
    {
        proc_(dwArgc, lpszArgv);
    }
};

template<class InputIt>
inline void service_control_dispatcher(InputIt first, InputIt last)
{
    std::vector<SERVICE_TABLE_ENTRYW> table;

    for (auto it = first; it != last; it++) {
        SERVICE_TABLE_ENTRYW svc = {
            const_cast<LPWSTR>((*it).name().c_str()),
            (*it).bootstrapper()
        };
        table.push_back(svc);
    }

    if (!table.size())
        return;
    else {
        SERVICE_TABLE_ENTRYW svc = {0};
        table.push_back(svc);
    }

    if (!::StartServiceCtrlDispatcherW(table.data())) {
        auto err = ::GetLastError();
        throw std::system_error(err, std::system_category());
    }
}

} // namespace win32

#endif // WIN32_SERVICE_HPP_INCLUDED
