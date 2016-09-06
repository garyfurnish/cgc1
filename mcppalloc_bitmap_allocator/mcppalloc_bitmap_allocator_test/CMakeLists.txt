include_directories(../../mcppalloc/mcppalloc/include)
include_directories(../../mcpputil/mcpputil/include)
include_directories(../../mcppalloc_bitmap/mcppalloc_bitmap/include)
include_directories(../../mcppalloc_slab_allocator/mcppalloc_slab_allocator/include)
include_directories(../mcppalloc_bitmap_allocator/include/)
find_path(BANDIT_INCLUDE_PATH bandit/bandit.h PATHS ../../bandit)
include_directories(${BANDIT_INCLUDE_PATH})
IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_compile_options(-fPIE)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_executable(mcppalloc_bitmap_allocator_test
  main.cpp
  bitmap.cpp
  )
target_link_libraries(mcppalloc_bitmap_allocator_test mcppalloc_slab_allocator)
