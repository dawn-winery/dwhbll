# dwhbll
> aka boost 0.5

Collection of the most random library components ever to come out of the Dawn Winery.

General list of library components:
- Logging functions
- Panics, todos, other useful features
- stl_ext sub-library
  - notably contains clones of Rust `Result<>` and `Option<>`
    - Including sugar for `Ok()`, `Err()`, `Some()`, and `None()`
    - Check `src/dwhbll/concurrency/coroutine/wrappers/sycall_wrappers.cpp` for usage
    - `sanify/types.h` or `sanify/stl_ext.h` includes relevant using namespace to avoid typing the leading namespace.
- Opinionated sanify library
  - Adds u8, i8 types, check sanify headers for specifics.
- Entire WIP C++26/29 compiler (dwcc-dev branch)
- Async runtime (WIP, c++20 coroutine, io_uring backend)
- Usable memory pool (needs work)
- Unicode library components, more are implemented on an as necessary basis, currently have just enough for
normalization required by the c++ standard.
- Collection of networking related things
- Some platform specific wrappers (notably linux ptrace)
- Subprocess tooling (similar to python popen) 
- Testing lib (uses C++26 reflection!)
- Trash drawer of other utilities in `utils`
