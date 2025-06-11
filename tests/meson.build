test_sources = [
  'test_main.cpp',
  'pool.cpp',
  'collections/ring.cpp',
  'matrix.cpp'
]


sub_tests = {
  'matrix': ['init']
}



dwhbll_test = executable('dwhbll_test',
  test_sources,
  include_directories : dwhbll_inc,
  link_with : dwhbll_lib
)

foreach s : test_sources 
  if s != 'test_main.cpp'
    test = s.split('.')[0]
    
    if sub_tests.has_key(test)
      foreach st : sub_tests[test]
        test(test + '(' + st + ')', dwhbll_test, args: [test, st])
      endforeach
    else
      test(test, dwhbll_test, args: [test])
    endif

  endif
endforeach
