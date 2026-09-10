# dwhbll
> aka boost 0.5

Collection of the most random library components ever to come out of the Dawn Winery.

General list of library components:
- Simple logging library with log filtering (`console/logging.h`)
- Debugging utilities (`debug/{debug,panic}.h`)
  - Rust-like `panic()` that prints stacktrace.
  - `ASSERT` macro that panics on fail (and does nothing in release).
  - `unreachable` (that panics in debug builds) and `todo` functions.
- stl_ext sub-library with a bunch of useful things (`stl_ext/*`)
  - clones of Rust's `Result` and `Option` (`stl_ext/{result,option}.h`)
    - Including sugar for `Ok()`, `Err()`, `Some()`, and `None()`.
    - Check `src/dwhbll/concurrency/coroutine/wrappers/sycall_wrappers.cpp` for usage.
    - `TRY` macros that behave similarly to Rust's `?` operator (`stl_ext/try.h`).
- Opinionated sanify library (`sanify/*`)
  - `u64`, `i64`, `u32`, etc. typedefs
  - Optional `using namespace` for some long namespace names.
- Entire WIP C++26/29 compiler in dwcc-dev branch (`lang/*`)
- Async runtime (WIP, C++20 coroutine, io_uring backend)
- Usable memory pool (needs work)
- Unicode library components, more are implemented on an as necessary basis, currently have just enough for
normalization required by the c++ standard.
- Collection of networking related things
- Simple JSON lib (`json/json.h`)
  - JSON trees are stored in a fairly stupid way, so depending on the usage it might be wasting a lot of memory.
- Some platform specific wrappers (notably linux ptrace)
- Subprocess tooling similar to python popen (`subprocess/process.h`)
- Reflection-based, Rust-like `dbg()` functions (`debug/format.h`)
  - For types that are not `std::formattable`, it recurses into it's members (where possible) to
  - Planning to have it support custom formatting functions.
try and print them. This can lead to a lot of noise from internal members, so it has a lot of room for improvement.
- WIP unit testing library and runner with C++26 reflection in reflection_tests branch (`testing/testing.h`)
  - Automatically discovers tests that have a `[[=test]]` annotation and runs them.
  - For usage examples see any of the `tests/*` files.
  - Also supports having "external" harnesses that are still managed by the main runner, to allow for
testing stuff that is hard to unit test, for example compilers or http servers.
