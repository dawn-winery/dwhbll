#include <dwhbll/utils/threading.h>

#include <cstring>

#include <unistd.h>

#include <dwhbll/console/debug.hpp>

namespace dwhbll::utils {
    void pin_thread_to_core(int core) {
        auto num_cores = sysconf(_SC_NPROCESSORS_ONLN);
        if (core < 0 || core >= num_cores)
            debug::panic("Attempting to pin to core {}, which is more than the available {} cores!", core, num_cores);

        cpu_set_t cpuset;

        CPU_ZERO(&cpuset);
        CPU_SET(core, &cpuset);

        pthread_t current_thread = pthread_self();
        if (pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset) < 0)
            debug::panic("Failed to pin to core {}! (msg: {})", core, strerror(errno));
    }
}
