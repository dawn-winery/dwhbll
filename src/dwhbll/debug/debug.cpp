#include <dwhbll/debug/debug.h>

#include <vector>
#include <fstream>


namespace dwhbll::debug {

#ifndef NDEBUG
thread_local std::vector<task_deferral*> _running_tasks;

const std::vector<task_deferral*>& running_tasks() {
    return _running_tasks;
}

task_deferral::task_deferral(const std::string &name) : name(name) {
    _running_tasks.push_back(this);
}

task_deferral::~task_deferral() {
    ASSERT(_running_tasks.back() == this);

    _running_tasks.pop_back();
}

const std::string & task_deferral::get_name() const {
    return name;
}
#endif

void cond_assert(bool condition) {
    cond_assert(condition, "");
}

bool is_being_debugged() {
    std::ifstream f("/proc/self/status");
    std::string line;
    while(std::getline(f, line)) {
        if (line.starts_with("TracerPid:")) {
            int tracer_pid = std::stoi(line.substr(10));
            return tracer_pid != 0;
        }
    }

    return false;
}

} // namespace dwhbll::debug
