include_directories(../../mcppalloc/mcppalloc/include)
include_directories(../../mcpputil/mcpputil/include)
include_directories(../../mcppalloc_slab_allocator/mcppalloc_slab_allocator/include)
include_directories(../mcppalloc_sparse/include/)
find_path(BANDIT_INCLUDE_PATH bandit/bandit.h PATHS ../../bandit)
include_directories(${BANDIT_INCLUDE_PATH})
IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_compile_options(-fPIE)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_executable(mcppalloc_sparse_test
  main.cpp
  allocator_block_tests.cpp 
  allocator_tests.cpp
  allocator_block_set_tests.cpp
  slab_allocator.cpp
  )
target_link_libraries(mcppalloc_sparse_test mcppalloc_slab_allocator mcpputil)
