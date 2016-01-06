include_directories(../../mcppalloc/mcppalloc/include)
include_directories(../mcppalloc_utils/include/)
find_path(BANDIT_INCLUDE_PATH bandit/bandit.h PATHS ../../bandit)
include_directories(${BANDIT_INCLUDE_PATH})
IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_compile_options(-fPIE)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_executable(mcppalloc_utils_test
  main.cpp
  )
target_link_libraries(mcppalloc_utils_test)
