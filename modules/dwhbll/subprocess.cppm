module;

#include <dwhbll/subprocess/process.h>
#include <dwhbll/subprocess/pipe_wrapper.h>

export module dwhbll.subprocess;

export namespace dwhbll::subprocess {
    using dwhbll::subprocess::returncode;
    using dwhbll::subprocess::process;
    using dwhbll::subprocess::popen;
    using dwhbll::subprocess::pipe_wrapper;
}
