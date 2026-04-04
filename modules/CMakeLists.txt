option(DWHBLL_MODULES_DNS2 "DNS Library Version 2" ON)

if (DWHBLL_MODULES_DNS2)
    message(STATUS "Including DNS2 Module")
    add_subdirectory(dns2)
endif()

option(DWHBLL_MODULES_HTTP "HTTP Client Library" ON)

if (DWHBLL_MODULES_HTTP)
    message(STATUS "Including HTTP Client Module")
    add_subdirectory(http)
endif()

option(DWHBLL_MODULES_SSL "SSL Library" ON)

if (DWHBLL_MODULES_SSL)
    message(STATUS "Including SSL Module")
    add_subdirectory(ssl)
endif()

set(DWHBLL_MODULE_FILE_SETS "${DWHBLL_MODULE_FILE_SETS}" PARENT_SCOPE)