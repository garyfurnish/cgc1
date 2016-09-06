include_directories(../mcppalloc/mcppalloc/include)
include_directories(../mcpputil/mcpputil/include)
include_directories(../mcppalloc_sparse/mcppalloc_sparse/include/)
include_directories(../mcppalloc_bitmap/mcppalloc_bitmap/include)
include_directories(../mcppalloc_slab_allocator/mcppalloc_slab_allocator/include)
include_directories(../mcppalloc_bitmap_allocator/mcppalloc_bitmap_allocator/include/)

include_directories(include)
IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  add_compile_options(-fPIC)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
list (APPEND SRC_FILES 
include/cgc1/cgc1.hpp
include/cgc1/cgc_internal_malloc_allocator.hpp
include/cgc1/declarations.hpp
src/gc_allocator.cpp
src/gc_allocator.hpp
src/gc_thread.cpp
src/gc_thread.hpp
src/global_kernel_state.cpp
src/global_kernel_state.hpp
src/global_kernel_state_impl.hpp
src/global_kernel_state_param.cpp
src/global_kernel_state_param.hpp
src/internal_allocator.hpp
src/internal_declarations.hpp
src/kernel.cpp
src/posix.cpp
src/thread_local_kernel_state.cpp
src/thread_local_kernel_state.hpp
src/thread_local_kernel_state_impl.hpp
src/util.cpp
)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
list (APPEND SRC_FILES ${SRC_FILES} include/cgc1/win32_slab.hpp
  include/cgc1/win32_slab_impl.hpp
  src/win32_slab.cpp)
ENDIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_library(cgc1 ${SRC_FILES})
add_library(gc SHARED ${SRC_FILES} src/new.cpp)
target_link_libraries(cgc1 mcppalloc_slab_allocator)
target_link_libraries(gc mcppalloc_slab_allocator)

IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
  target_link_libraries(cgc1 pthread)
  target_link_libraries(gc pthread)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
INSTALL(DIRECTORY "include/cgc1" DESTINATION "include")
INSTALL(DIRECTORY "include/gc" DESTINATION "include")
INSTALL(FILES "include/gc.h" DESTINATION "include")
INSTALL(TARGETS cgc1
                RUNTIME DESTINATION bin
                LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib)
INSTALL(TARGETS gc
                RUNTIME DESTINATION bin
                LIBRARY DESTINATION lib
                ARCHIVE DESTINATION lib)