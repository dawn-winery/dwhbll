#pragma once

#define TRACE_FUNC(fmt, ...) ::dwhbll::console::trace("[{}:{}][{}] " fmt, __FILE_NAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define DEBUG_FUNC(fmt, ...) ::dwhbll::console::debug("[{}:{}][{}] " fmt, __FILE_NAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define INFO_FUNC(fmt, ...) ::dwhbll::console::info("[{}:{}][{}] " fmt, __FILE_NAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define WARN_FUNC(fmt, ...) ::dwhbll::console::warn("[{}:{}][{}] " fmt, __FILE_NAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define ERROR_FUNC(fmt, ...) ::dwhbll::console::error("[{}:{}][{}] " fmt, __FILE_NAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define CRITICAL_FUNC(fmt, ...) ::dwhbll::console::critical("[{}:{}][{}] " fmt, __FILE_NAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
#define FATAL_FUNC(fmt, ...) ::dwhbll::console::fatal("[{}:{}][{}] " fmt, __FILE_NAME__, __LINE__, __FUNCTION__, ##__VA_ARGS__)
