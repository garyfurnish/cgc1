include_directories(../mcppalloc/mcppalloc/include)
include_directories(../mcppalloc_utils/mcppalloc_utils/include)
include_directories(../mcppalloc_sparse/mcppalloc_sparse/include/)
include_directories(../mcppalloc_bitmap/mcppalloc_bitmap/include)
include_directories(../mcppalloc_slab_allocator/mcppalloc_slab_allocator/include)
include_directories(../mcppalloc_bitmap_allocator/mcppalloc_bitmap_allocator/include/)
include_directories(../cgc1/include)
find_path(BANDIT_INCLUDE_PATH bandit/bandit.h PATHS ../bandit)
include_directories(${BANDIT_INCLUDE_PATH})
IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_compile_options(-fPIE)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_executable(cgc1_test
  main.cpp
  gc_tests.cpp
  gc_sparse_allocator_tests.cpp
  bitmap_allocator_tests.cpp
  )
target_link_libraries(cgc1_test cgc1)
