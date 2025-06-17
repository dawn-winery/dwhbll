#include <dwhbll/console/Logging.h>

#include <iostream>
#include <unordered_map>

namespace dwhbll::console {
    Level defaultLevel = Level::INFO;
    Level cerrLevel = Level::ERROR;
    bool colors = true;

    std::unordered_map<Level, std::string> levelsToString = {
        {Level::TRACE, "TRACE"},
        {Level::DEBUG, "DEBUG"},
        {Level::INFO, "INFO "},
        {Level::WARN, "WARN "},
        {Level::ERROR, "ERROR"},
        {Level::CRITICAL, "CRIT "},
        {Level::FATAL, "FATAL"},
        {Level::NONE, "NONE "},
    };

    void log(const std::string &msg, const Level level) {
        if (level < defaultLevel)
            return;
        if (level >= cerrLevel)
            std::cerr << "[" << levelsToString[level] << "]" << msg << std::endl;
        else
            std::cout << "[" << levelsToString[level] << "]" << msg << std::endl;
    }

    void fatal(const std::string &msg) {
        log(msg, Level::FATAL);
    }

    void critical(const std::string &msg) {
        log(msg, Level::CRITICAL);
    }

    void error(const std::string &msg) {
        log(msg, Level::ERROR);
    }

    void warn(const std::string &msg) {
        log(msg, Level::WARN);
    }

    void info(const std::string &msg) {
        log(msg, Level::INFO);
    }

    void debug(const std::string &msg) {
        log(msg, Level::DEBUG);
    }

    void trace(const std::string &msg) {
        log(msg, Level::TRACE);
    }
}
