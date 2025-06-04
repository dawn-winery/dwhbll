test_sources = [
  'test_main.cpp',
  'pool.cpp'
]

dwhbll_test = executable('dwhbll_test',
  test_sources,
  include_directories : dwhbll_inc,
  link_with : dwhbll_lib
)

test('pool', dwhbll_test)
