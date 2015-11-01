include_directories(../cgc1/include)
find_path(BANDIT_INCLUDE_PATH bandit/bandit.h ../bandit)
include_directories(${BANDIT_INCLUDE_PATH})
IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_compile_options(-fPIE)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_executable(cgc1_test
  main.cpp
  gc_tests.cpp
  gc_sparse_allocator_tests.cpp
  packed_object.cpp
  slab_allocator.cpp
  allocator_block_tests.cpp 
  allocator_tests.cpp
  allocator_block_set_tests.cpp
  )
target_link_libraries(cgc1_test cgc1)
