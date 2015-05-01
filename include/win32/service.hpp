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
#include <utility>
#include <memory>

#include <cinttypes>
#include <cstring>

#include <windows.h>

namespace win32 {

enum class service_status : unsigned long {
    stopped             = SERVICE_STOPPED,          // 1
    start_pending       = SERVICE_START_PENDING,    // 2
    stop_pending        = SERVICE_STOP_PENDING,     // 3
    running             = SERVICE_RUNNING,          // 4
    continue_pending    = SERVICE_CONTINUE_PENDING, // 5
    pause_pending       = SERVICE_PAUSE_PENDING,    // 6
    paused              = SERVICE_PAUSED            // 7
};

enum class service_type : unsigned long {
    kernel_driver       = SERVICE_KERNEL_DRIVER,        // 0x001
    file_system_driver  = SERVICE_FILE_SYSTEM_DRIVER,   // 0x002
    win32_own_process   = SERVICE_WIN32_OWN_PROCESS,    // 0x010
    win32_share_process = SERVICE_WIN32_SHARE_PROCESS,  // 0x020
    interactive_process = SERVICE_INTERACTIVE_PROCESS,  // 0x100
};

enum class service_controls_accept : unsigned long {
    stop                    = SERVICE_ACCEPT_STOP,                      // 0x001
    pause_continue          = SERVICE_ACCEPT_PAUSE_CONTINUE,            // 0x002
    shutdown                = SERVICE_ACCEPT_SHUTDOWN,                  // 0x004
    paramchange             = SERVICE_ACCEPT_PARAMCHANGE,               // 0x008
    netbindchange           = SERVICE_ACCEPT_NETBINDCHANGE,             // 0x010
    hardwareprofilechange   = SERVICE_ACCEPT_HARDWAREPROFILECHANGE,     // 0x020
    powerevent              = SERVICE_ACCEPT_POWEREVENT,                // 0x040
    sessionchange           = SERVICE_ACCEPT_SESSIONCHANGE,             // 0x080
    preshutdown             = SERVICE_ACCEPT_PRESHUTDOWN,               // 0x100
    timechange              = SERVICE_ACCEPT_TIMECHANGE,                // 0x200
    triggerevent            = SERVICE_ACCEPT_TRIGGEREVENT,              // 0x400
};

inline service_controls_accept operator | (
        service_controls_accept lhs,
        service_controls_accept rhs)
{
    return static_cast<service_controls_accept>(
            static_cast<unsigned long>(lhs) | static_cast<unsigned long>(rhs));
}

class service;
class service_controller;

typedef std::function<unsigned long(
        const std::shared_ptr<service_controller>&,
        unsigned long,
        unsigned long,
        void *)> service_control_handler;

class service_control_handler_context final {
public:
    service_control_handler_context(const service_control_handler& h) : h_(h)
    {
    }

    const service_control_handler& handler() const
    {
        return h_;
    }

    void service_controller(
            const std::shared_ptr<win32::service_controller>& ctl)
    {
        ctl_ = ctl;
    }

    const std::shared_ptr<win32::service_controller>& service_controller() const
    {
        return ctl_;
    }
private:
    service_control_handler h_;
    std::shared_ptr<win32::service_controller> ctl_;
};

#ifdef _M_AMD64
/*
    sub rsp, 24
    mov r8, rdx
    mov edx, ecx
    mov rcx, 0FFFFFFFFFFFFFFFFh ; this
    mov rax, 0FFFFFFFFFFFFFFFFh ; proc
    call rax
    add rsp, 24
    ret
 */
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

typedef std::function<void(const service&, int, wchar_t **)> service_procedure;

class service final {
public:
    service(
            const std::wstring& name,
            const service_procedure& proc) :
                    name_(name),
                    proc_(proc)
    {
        proctrunk_ = reinterpret_cast<std::uint8_t *>(VirtualAlloc(
                nullptr,
                sizeof(service_main_trunk),
                MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE));

        if (!proctrunk_) {
            auto err = GetLastError();
            throw std::system_error(err, std::system_category());
        }

        std::memcpy(proctrunk_, service_main_trunk, sizeof(service_main_trunk));
        reinterpret_cast<std::intptr_t *>(
                proctrunk_ + SERVICE_MAIN_TRUNK_THIS_OFFSET)[0] =
                reinterpret_cast<std::intptr_t>(this);

        union {
            VOID (WINAPI service::*ptr)(DWORD, LPWSTR *);
            std::intptr_t addr;
        } svcmain = {&service::service_boot};

        reinterpret_cast<std::intptr_t *>(
                proctrunk_ + SERVICE_MAIN_TRUNK_PROC_OFFSET)[0] = svcmain.addr;
    }

    service(service&& src) :
            name_(std::move(src.name_)),
            proc_(std::move(src.proc_)),
            proctrunk_(src.proctrunk_)
    {
        src.proctrunk_ = nullptr;
        reinterpret_cast<std::intptr_t *>(
                proctrunk_ + SERVICE_MAIN_TRUNK_THIS_OFFSET)[0] =
                reinterpret_cast<std::intptr_t>(this);
    }

    service(const service&) = delete;

    ~service()
    {
        if (proctrunk_) {
            VirtualFree(proctrunk_, 0, MEM_RELEASE);
        }
    }

    service& operator = (service&& src)
    {
        if (proctrunk_) {
            VirtualFree(proctrunk_, 0, MEM_RELEASE);
        }

        name_ = std::move(src.name_);
        proc_ = std::move(src.proc_);
        proctrunk_ = src.proctrunk_;
        src.proctrunk_ = nullptr;

        reinterpret_cast<std::intptr_t *>(
                proctrunk_ + SERVICE_MAIN_TRUNK_THIS_OFFSET)[0] =
                reinterpret_cast<std::intptr_t>(this);

        return *this;
    }

    service& operator = (const service&) = delete;

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
    service_procedure proc_;
    std::uint8_t *proctrunk_;

    void service_boot(DWORD argc, LPWSTR *argv)
    {
        proc_(*this, argc, argv);
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

    if (!StartServiceCtrlDispatcherW(table.data())) {
        auto err = GetLastError();
        throw std::system_error(err, std::system_category());
    }
}

class service_controller final :
        public std::enable_shared_from_this<service_controller> {
public:
    service_controller(
            const win32::service& svc,
            SERVICE_STATUS_HANDLE sth,
            service_type svctype,
            service_controls_accept ctls,
            unsigned long inittime,
            service_control_handler_context *hctx) :
                    svc_(svc),
                    sth_(sth),
                    hctx_(hctx),
                    ctls_(ctls)
    {
        std::memset(&st_, 0, sizeof(st_));
        st_.dwServiceType = static_cast<DWORD>(svctype);
        st_.dwCurrentState = static_cast<DWORD>(service_status::start_pending);
        st_.dwCheckPoint = 1;
        st_.dwWaitHint = inittime;

        if (!SetServiceStatus(sth_, &st_)) {
            throw std::system_error(GetLastError(), std::system_category());
        }
    }

    service_controller(const service_controller&) = delete;

    ~service_controller()
    {
        delete hctx_;
    }

    service_controller& operator = (const service_controller&) = delete;

    void begin_continue(unsigned long t)
    {
        st_.dwCurrentState = static_cast<DWORD>(
                service_status::continue_pending);
        st_.dwCheckPoint = 1;
        st_.dwWaitHint = t;

        if (!SetServiceStatus(sth_, &st_)) {
            throw std::system_error(GetLastError(), std::system_category());
        }
    }

    void begin_pause(unsigned long t)
    {
        st_.dwCurrentState = static_cast<DWORD>(service_status::pause_pending);
        st_.dwCheckPoint = 1;
        st_.dwWaitHint = t;

        if (!SetServiceStatus(sth_, &st_)) {
            throw std::system_error(GetLastError(), std::system_category());
        }
    }

    void begin_stop(unsigned long t)
    {
        st_.dwCurrentState = static_cast<DWORD>(service_status::stop_pending);
        st_.dwCheckPoint = 1;
        st_.dwWaitHint = t;

        if (!SetServiceStatus(sth_, &st_)) {
            throw std::system_error(GetLastError(), std::system_category());
        }
    }

    void continued()
    {
        st_.dwCurrentState = static_cast<DWORD>(service_status::running);
        st_.dwCheckPoint = 0;
        st_.dwWaitHint = 0;

        if (!SetServiceStatus(sth_, &st_)) {
            throw std::system_error(GetLastError(), std::system_category());
        }
    }

    void finish_init()
    {
        hctx_->service_controller(shared_from_this());

        st_.dwCurrentState = static_cast<DWORD>(service_status::running);
        st_.dwControlsAccepted = static_cast<DWORD>(
                service_controls_accept::stop | ctls_);
        st_.dwCheckPoint = 0;
        st_.dwWaitHint = 0;

        if (!SetServiceStatus(sth_, &st_)) {
            auto e = GetLastError();
            hctx_->service_controller(nullptr);
            throw std::system_error(e, std::system_category());
        }
    }

    void increase_pending_progress()
    {
        st_.dwCheckPoint++;

        if (!SetServiceStatus(sth_, &st_)) {
            throw std::system_error(GetLastError(), std::system_category());
        }
    }

    void paused()
    {
        st_.dwCurrentState = static_cast<DWORD>(service_status::paused);
        st_.dwCheckPoint = 0;
        st_.dwWaitHint = 0;

        if (!SetServiceStatus(sth_, &st_)) {
            throw std::system_error(GetLastError(), std::system_category());
        }
    }

    const win32::service& service() const
    {
        return svc_;
    }

    void stopped(unsigned long e = NO_ERROR, bool custom_err = false)
    {
        st_.dwCurrentState = static_cast<DWORD>(service_status::stopped);
        st_.dwCheckPoint = 0;
        st_.dwWaitHint = 0;

        if (custom_err) {
            st_.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
            st_.dwServiceSpecificExitCode = e;
        } else {
            st_.dwWin32ExitCode = e;
            st_.dwServiceSpecificExitCode = 0;
        }

        if (!SetServiceStatus(sth_, &st_)) {
            throw std::system_error(GetLastError(), std::system_category());
        }
    }
private:
    const win32::service& svc_;
    service_control_handler_context *hctx_;
    service_controls_accept ctls_;
    SERVICE_STATUS_HANDLE sth_;
    SERVICE_STATUS st_;
};

inline DWORD WINAPI service_control_handler_trunk(
        DWORD ctl,
        DWORD evt,
        LPVOID evt_data,
        LPVOID ctx)
{
    auto hctx = reinterpret_cast<service_control_handler_context *>(ctx);
    unsigned long result;

    try {
        result = hctx->handler()(
                hctx->service_controller(),
                ctl,
                evt,
                evt_data);
    } catch (...) {
        if (ctl == SERVICE_CONTROL_STOP) {
            hctx->service_controller(nullptr);
        }
        throw;
    }

    if (ctl == SERVICE_CONTROL_STOP) {
        hctx->service_controller(nullptr);
    }

    return result;
}

inline std::shared_ptr<service_controller> register_service_control_handler(
        const service& svc,
        service_type svctype,
        service_controls_accept svcctls,
        unsigned long inittime,
        const service_control_handler& h)
{
    auto ctx = new service_control_handler_context(h);
    auto sth = ::RegisterServiceCtrlHandlerExW(
            svc.name().c_str(),
            service_control_handler_trunk,
            ctx);

    if (!sth) {
        auto e = ::GetLastError();
        delete ctx;
        throw std::system_error(e, std::system_category());
    }

    try {
        return std::make_shared<service_controller>(
                svc,
                sth,
                svctype,
                svcctls,
                inittime,
                ctx);
    } catch (...) {
        delete ctx;
        throw;
    }
}

} // namespace win32

#endif // WIN32_SERVICE_HPP_INCLUDED
