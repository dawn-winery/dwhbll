#pragma once

#include <dwhbll/concurrency/coroutine/task.h>
#include <dwhbll/concurrency/coroutine/sleep_task.h>
#include <dwhbll/concurrency/coroutine/reactor.h>

#define yield co_yield
#define await co_await
#define finish co_return

using namespace dwhbll::concurrency::coroutine;
