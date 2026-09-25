module;

#include <dwhbll/exceptions/rt_exception_base.h>
#include <dwhbll/exceptions/sys_error.h>
#include <dwhbll/exceptions/timeout_exception.h>
#include <dwhbll/exceptions/concurrency_exception.h>

export module dwhbll.exceptions;

export namespace dwhbll::exceptions {
    using dwhbll::exceptions::rt_exception_base;
    using dwhbll::exceptions::sys_error;
    using dwhbll::exceptions::timeout_exception;
    using dwhbll::exceptions::concurrency_exception;
}
