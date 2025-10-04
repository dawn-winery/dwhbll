project('dwhbll', 'cpp', default_options: ['cpp_std=c++23'])

dwhbll_inc = include_directories('include')

dwhbll_sources = [
  'src' / 'dwhbll' / 'collections' / 'cache.cpp',
  'src' / 'dwhbll' / 'console' / 'debug.cpp',
  'src' / 'dwhbll' / 'console' / 'Logging.cpp',
  'src' / 'dwhbll' / 'exceptions' / 'rt_exception_base.cpp',
  'src' / 'dwhbll' / 'graphics' / 'bitmap.cpp',
  'src' / 'dwhbll' / 'network' / 'dns' / 'dns.cpp',
  'src' / 'dwhbll' / 'network' / 'SocketManager.cpp',
  'src' / 'dwhbll' / 'subprocess' / 'pipe_wrapper.cpp',
  'src' / 'dwhbll' / 'subprocess' / 'process.cpp',
  'src' / 'dwhbll' / 'utils' / 'json.cpp',
  'src' / 'dwhbll' / 'utils' / 'utils.cpp',
]

dwhbll_lib = static_library('dwhbll', dwhbll_sources, include_directories: dwhbll_inc)

install_subdir('include', install_dir: get_option('includedir'))

subdir('tests')
