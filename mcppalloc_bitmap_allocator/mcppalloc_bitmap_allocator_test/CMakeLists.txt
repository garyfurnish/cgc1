include_directories(../../mcppalloc/mcppalloc/include)
include_directories(../../mcppalloc_utils/mcppalloc_utils/include)
include_directories(../../mcppalloc_bitmap/mcppalloc_bitmap/include)
include_directories(../../mcppalloc_slab_allocator/mcppalloc_slab_allocator/include)
include_directories(../mcppalloc_bitmap_allocator/include/)
find_path(BANDIT_INCLUDE_PATH bandit/bandit.h ../bandit)
include_directories(${BANDIT_INCLUDE_PATH})
IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_compile_options(-fPIE)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_executable(mcppalloc_bitmap_allocator_test
  main.cpp
  bitmap.cpp
  )
target_link_libraries(mcppalloc_bitmap_allocator_test)
