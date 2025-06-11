project('dwhbll', 'cpp', default_options: ['cpp_std=c++23'])

dwhbll_inc = include_directories('include')

dwhbll_sources = [
  'include/dwhbll/memory/pool.h',
  'include/dwhbll/linalg/matrix.h'
]

dwhbll_lib = static_library('dwhbll',
  dwhbll_sources,
  include_directories : dwhbll_inc
)

subdir('tests')

install_subdir('include', install_dir: get_option('includedir'))
