project('dwhbll', 'cpp', default_options: ['cpp_std=c++23'])

dwhbll_inc = include_directories('include')

dwhbll_sources = [
  'src/dwhbll/collections/cache.cpp',
  'src/dwhbll/console/Logging.cpp',
  'src/dwhbll/network/SocketManager.cpp',
  'src/dwhbll/network/dns/dns.cpp'
]

dwhbll_lib = static_library('dwhbll',
  dwhbll_sources,
  include_directories : dwhbll_inc
)

install_subdir('include', install_dir: get_option('includedir'))

subdir('tests')
