test_sources = [
  'collections' / 'cache.cpp',
  'collections' / 'ring.cpp',
  'collections' / 'streams.cpp',
  'graphics' / 'bitmap.cpp',
  'matrix.cpp',
  'pool.cpp',
  'test_main.cpp',
]

sub_tests = {
  'collections/streams': ['MemoryBuffer', 'FileBuffer', 'StreamReader', 'CachedReader'],
  'graphics/bitmap': ['fractal'],
  'matrix': ['init', 'mul'],
}

dwhbll_test = executable(
  'dwhbll_test',
  test_sources,
  include_directories: dwhbll_inc,
  link_with: dwhbll_lib,
)

foreach s : test_sources
  if s != 'test_main.cpp'
    test = s.split('.')[0]

    if sub_tests.has_key(test)
      foreach st : sub_tests[test]
        test(test + '(' + st + ')', dwhbll_test, timeout: 0, args: [test, st])
      endforeach
    else
      test(test, dwhbll_test, timeout: 0, args: [test])
    endif

  endif
endforeach
