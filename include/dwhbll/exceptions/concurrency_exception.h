#pragma once

#include <dwhbll/exceptions/rt_exception_base.h>

namespace dwhbll::exceptions {
    class concurrency_exception : public rt_exception_base {
    public:
        using rt_exception_base::rt_exception_base;
    };
}
