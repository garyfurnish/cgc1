include_directories(../../mcppalloc/mcppalloc/include)
include_directories(../../mcpputil/mcpputil/include)

include_directories(include)
IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  add_compile_options(-fPIC)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_library(mcppalloc_slab_allocator
  include/mcppalloc/mcppalloc_slab_allocator/slab_allocator.hpp
  include/mcppalloc/mcppalloc_slab_allocator/slab_allocator_impl.hpp
  src/slab_allocator.cpp
)
INSTALL(DIRECTORY "include/mcppalloc" DESTINATION "include")
INSTALL(TARGETS mcppalloc_slab_allocator
                RUNTIME DESTINATION bin
                LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib)
