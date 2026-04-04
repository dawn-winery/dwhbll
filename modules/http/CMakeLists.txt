target_sources(dwhbll_modules
    PUBLIC
    FILE_SET http_module TYPE CXX_MODULES FILES
        http.ixx
)

list(APPEND DWHBLL_MODULE_FILE_SETS http_module)
set(DWHBLL_MODULE_FILE_SETS "${DWHBLL_MODULE_FILE_SETS}" PARENT_SCOPE)
