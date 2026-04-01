#pragma once

#include <span>
#include <sched.h>
#include <csignal>
#include <sys/ptrace.h>
#include <sys/user.h>

#include <dwhbll/stl_ext/cow.h>

namespace dwhbll::platform::linux_wrappers {
    struct PTraceFlags {
        bool ExitKill{false};
        bool TraceClone{false};
        bool TraceExec{false};
        bool TraceExit{false};
        bool TraceFork{false};
        bool TraceSysGood{false};
        bool TraceVFork{false};
        bool TraceVForkDone{false};
        bool TraceSeccomp{false};
        bool SuspendSeccomp{false};
    };

    class PTrace {
        stl_ext::cow<PTraceFlags> flags{{}};

        pid_t pid;

        int memfd = -1;

        static void check(long status);

    public:
        explicit PTrace(pid_t pid);

        /**
         * @brief sets ptrace traceme, such that a tracer can trace the current process.
         */
        static void traceme();

        void readMemory(std::uint64_t addr, std::span<std::uint8_t> data);

        void writeMemory(std::uint64_t addr, std::span<std::uint8_t> data);

        [[nodiscard]] user_regs_struct getRegs() const;

        void setRegs(user_regs_struct regs) const;

        [[nodiscard]] user_fpregs_struct getFPRegs() const;

        void setFPRegs(user_fpregs_struct regs) const;

        /*
         * TODO: Missing wrappers as of yet:
         * PEEKTEXT, PEEKDATA, PEEKUSER,
         * POKETEXT, POKEDATA, POKEUSER,
         * GETREGSET
         * SETREGSET
         * GETSIGINFO, SETSIGINFO, PEEKSIGINFO,
         * GETEVENTMSG,
         *
         * SET_SYSCALL(modify call number reg on x86),
         *
         * SECCOMP_GET_FILTER,
         * GET_THREAD_AREA, SET_THREAD_AREA,
         * GET_SYSCALL_INFO,
         */

        void seize() const;

        void attach() const;

        // [[nodiscard]] bool checkStillAlive() const;

        void detach() const;

        void resume() const;

        void resumeWithSignal(int signal) const;

        void resumeUntilSyscall() const;

        void resumeUntilSyscallWithSignal(int signal) const;

        void resumeSinglestep() const;

        void resumeSinglestepWithSignal(int signal) const;

        void resumeSysemu() const;

        void resumeSysemuWithSignal(int signal) const;

        void resumeSysemuSinglestep() const;

        void resumeSysemuSinglestepWithSignal(int signal) const;

        void resumeListen() const;

        void stopInterrupt() const;

        void sendSignal(int signal) const;

        void killChild() const;

        [[nodiscard]] sigset_t getSigMask() const;

        void setSigMask(sigset_t signals) const;

        [[nodiscard]] bool getOptExitKill() const;

        void setOptExitKill();

        [[nodiscard]] bool getOptTraceClone() const;

        void setOptTraceClone();

        [[nodiscard]] bool getOptTraceExec() const;

        void setOptTraceExec();

        [[nodiscard]] bool getOptTraceExit() const;

        void setOptTraceExit();

        [[nodiscard]] bool getOptTraceFork() const;

        void setOptTraceFork();

        [[nodiscard]] bool getOptTraceSysGood() const;

        void setOptTraceSysGood();

        [[nodiscard]] bool getOptTraceVFork() const;

        void setOptTraceVFork();

        [[nodiscard]] bool getOptTraceVForkDone() const;

        void setOptTraceVForkDone();

        [[nodiscard]] bool getOptTraceSeccomp() const;

        void setOptTraceSeccomp();

        [[nodiscard]] bool getSuspendSeccomp() const;

        void setSuspendSeccomp();
    };
}
