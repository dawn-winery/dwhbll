target_sources(dwhbll_modules
PUBLIC
FILE_SET dns2_module TYPE CXX_MODULES FILES
    dns2.ixx
    proto.ixx
)

list(APPEND DWHBLL_MODULE_FILE_SETS dns2_module)
set(DWHBLL_MODULE_FILE_SETS "${DWHBLL_MODULE_FILE_SETS}" PARENT_SCOPE)
