#include <dwhbll/platform/linux_wrappers/ptrace.h>

#include <cstring>
#include <fcntl.h>

#include <dwhbll/console/debug.hpp>
#include <dwhbll/exceptions/rt_exception_base.h>

namespace dwhbll::platform::linux_wrappers {
    void PTrace::check(long status) {
        if (status == -1) {
            throw exceptions::rt_exception_base("PTrace Operation Failed! ({})", strerror(errno));
        }
    }

    PTrace::PTrace(const pid_t pid) : pid(pid) {
        ASSERT(pid != getpid());
    }

    void PTrace::traceme() {
        check(ptrace(PTRACE_TRACEME, 0, nullptr, nullptr));
    }

    void PTrace::readMemory(std::uint64_t addr, std::span<std::uint8_t> data) {
        if (memfd == -1) {
            std::string target = std::format("/proc/{}/mem", pid);
            memfd = open(target.c_str(), O_RDWR);

            if (memfd == -1)
                throw exceptions::rt_exception_base("Failed to open proc mem!");
        }

        check(lseek(memfd, addr, SEEK_SET));

        auto count = read(memfd, data.data(), data.size());

        if (count >= data.size())
            debug::panic("Count >= buffer");

        if (count != data.size())
            throw exceptions::rt_exception_base("Out of bounds read.");
    }

    void PTrace::writeMemory(std::uint64_t addr, std::span<std::uint8_t> data) {
        if (memfd == -1) {
            std::string target = std::format("/proc/{}/mem", pid);
            memfd = open(target.c_str(), O_RDWR);

            if (memfd == -1)
                throw exceptions::rt_exception_base("Failed to open proc mem!");
        }

        check(lseek(memfd, addr, SEEK_SET));

        auto count = write(memfd, data.data(), data.size());

        if (count >= data.size())
            debug::panic("Count >= buffer");

        if (count != data.size())
            throw exceptions::rt_exception_base("Out of bounds write.");
    }

    user_regs_struct PTrace::getRegs() const {
        user_regs_struct reg{};

        check(ptrace(PTRACE_GETREGS, pid, nullptr, &reg));

        return reg;
    }

    void PTrace::setRegs(user_regs_struct regs) const {
        check(ptrace(PTRACE_SETREGS, pid, nullptr, &regs));
    }

    user_fpregs_struct PTrace::getFPRegs() const {
        user_fpregs_struct reg{};

        check(ptrace(PTRACE_GETFPREGS, pid, nullptr, &reg));

        return reg;
    }

    void PTrace::setFPRegs(user_fpregs_struct regs) const {
        check(ptrace(PTRACE_SETFPREGS, pid, nullptr, &regs));
    }

    void PTrace::seize() const {
        check(ptrace(PTRACE_SEIZE, pid, nullptr, nullptr));
    }

    void PTrace::attach() const {
        check(ptrace(PTRACE_ATTACH, pid, nullptr, nullptr));
    }

    void PTrace::detach() const {
        check(ptrace(PTRACE_DETACH, pid, nullptr, nullptr));
    }

    void PTrace::resume() const {
        check(ptrace(PTRACE_CONT, pid, nullptr, nullptr));
    }

    void PTrace::resumeWithSignal(const int signal) const {
        check(ptrace(PTRACE_CONT, pid, nullptr, signal));
    }

    void PTrace::resumeUntilSyscall() const {
        check(ptrace(PTRACE_SYSCALL, pid, nullptr, nullptr));
    }

    void PTrace::resumeUntilSyscallWithSignal(const int signal) const {
        check(ptrace(PTRACE_SYSCALL, pid, nullptr, signal));
    }

    void PTrace::resumeSinglestep() const {
        check(ptrace(PTRACE_SINGLESTEP, pid, nullptr, nullptr));
    }

    void PTrace::resumeSinglestepWithSignal(const int signal) const {
        check(ptrace(PTRACE_SINGLESTEP, pid, nullptr, signal));
    }

    void PTrace::resumeSysemu() const {
#if !defined(__x86_64__) && !defined(__i386__)
        debug::panic("Sysemu is not a supported operation on non x86 based machines!")
#else
        check(ptrace(PTRACE_SYSEMU, pid, nullptr, nullptr));
#endif
    }

    void PTrace::resumeSysemuWithSignal(const int signal) const {
#if !defined(__x86_64__) && !defined(__i386__)
        debug::panic("Sysemu is not a supported operation on non x86 based machines!")
#else
        check(ptrace(PTRACE_SYSEMU, pid, nullptr, signal));
#endif
    }

    void PTrace::resumeSysemuSinglestep() const {
#if !defined(__x86_64__) && !defined(__i386__)
        debug::panic("Sysemu singlestep is not a supported operation on non x86 based machines!")
#else
        check(ptrace(PTRACE_SYSEMU_SINGLESTEP, pid, nullptr, nullptr));
#endif
    }

    void PTrace::resumeSysemuSinglestepWithSignal(const int signal) const {
#if !defined(__x86_64__) && !defined(__i386__)
        debug::panic("Sysemu singlestep is not a supported operation on non x86 based machines!")
#else
        check(ptrace(PTRACE_SYSEMU_SINGLESTEP, pid, nullptr, signal));
#endif
    }

    void PTrace::resumeListen() const {
        check(ptrace(PTRACE_LISTEN, pid, nullptr, nullptr));
    }

    void PTrace::stopInterrupt() const {
        check(ptrace(PTRACE_INTERRUPT, pid, nullptr, nullptr));
    }

    void PTrace::sendSignal(int signal) const {
        kill(pid, signal);
    }

    void PTrace::killChild() const {
        kill(pid, SIGKILL);
    }

    sigset_t PTrace::getSigMask() const {
        sigset_t set{};

        ptrace(PTRACE_GETSIGMASK, pid, sizeof(sigset_t), &set);

        return set;
    }

    void PTrace::setSigMask(sigset_t signals) const {
        check(ptrace(PTRACE_SETSIGMASK, pid, sizeof(sigset_t), &signals));
    }

    bool PTrace::getOptExitKill() const {
        return flags->ExitKill;
    }

    void PTrace::setOptExitKill() {
        if (flags->ExitKill)
            return;

        check(ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_EXITKILL));

        flags.mut().ExitKill = true;
    }

    bool PTrace::getOptTraceClone() const {
        return flags->TraceClone;
    }

    void PTrace::setOptTraceClone() {
        if (flags->TraceClone)
            return;

        check(ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_TRACECLONE));

        flags.mut().TraceClone = true;
    }

    bool PTrace::getOptTraceExec() const {
        return flags->TraceExec;
    }

    void PTrace::setOptTraceExec() {
        if (flags->TraceExec)
            return;

        check(ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_TRACEEXEC));

        flags.mut().TraceExec = true;
    }

    bool PTrace::getOptTraceExit() const {
        return flags->TraceExit;
    }

    void PTrace::setOptTraceExit() {
        if (flags->TraceExit)
            return;

        check(ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_TRACEEXIT));

        flags.mut().TraceExit = true;
    }

    bool PTrace::getOptTraceFork() const {
        return flags->TraceFork;
    }

    void PTrace::setOptTraceFork() {
        if (flags->TraceFork)
            return;

        check(ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_TRACEFORK));

        flags.mut().TraceFork = true;
    }

    bool PTrace::getOptTraceSysGood() const {
        return flags->TraceSysGood;
    }

    void PTrace::setOptTraceSysGood() {
        if (flags->TraceSysGood)
            return;

        check(ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_TRACESYSGOOD));

        flags.mut().TraceSysGood = true;
    }

    bool PTrace::getOptTraceVFork() const {
        return flags->TraceVFork;
    }

    void PTrace::setOptTraceVFork() {
        if (flags->TraceVFork)
            return;

        check(ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_TRACEVFORK));

        flags.mut().TraceVFork = true;
    }

    bool PTrace::getOptTraceVForkDone() const {
        return flags->TraceVForkDone;
    }

    void PTrace::setOptTraceVForkDone() {
        if (flags->TraceVForkDone)
            return;

        check(ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_TRACEVFORKDONE));

        flags.mut().TraceVForkDone = true;
    }

    bool PTrace::getOptTraceSeccomp() const {
        return flags->TraceSeccomp;
    }

    void PTrace::setOptTraceSeccomp() {
        if (flags->TraceSeccomp)
            return;

        check(ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_TRACESECCOMP));

        flags.mut().TraceSeccomp = true;
    }

    bool PTrace::getSuspendSeccomp() const {
        return flags->SuspendSeccomp;
    }

    void PTrace::setSuspendSeccomp() {
        if (flags->SuspendSeccomp)
            return;

        check(ptrace(PTRACE_SETOPTIONS, pid, nullptr, PTRACE_O_SUSPEND_SECCOMP));

        flags.mut().SuspendSeccomp = true;
    }
}
