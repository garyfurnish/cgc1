#INSTALL(DIRECTORY include/ DESTINATION include)
IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_compile_options(-fno-rtti)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_library(cgc1
src/allocator.hpp                 src/internal_allocator.hpp
src/allocator_block.hpp           src/internal_declarations.hpp
src/allocator_block_impl.hpp      src/kernel.cpp
src/allocator_block_set.hpp       src/object_state.hpp
src/allocator_block_set_impl.hpp  src/posix.cpp
src/allocator_impl.hpp            src/slab_allocator.hpp
src/gc_allocator.hpp              src/thread_allocator.hpp
src/gc_thread.cpp                 src/thread_allocator_impl.hpp
src/gc_thread.hpp                 src/thread_local_kernel_state.cpp
src/global_kernel_state.cpp       src/thread_local_kernel_state.hpp
src/global_kernel_state.hpp       src/thread_local_kernel_state_impl.hpp
src/global_kernel_state_impl.hpp  src/win32_slab.cpp
src/slab_allocator.cpp src/object_state.cpp
src/slab_allocator_impl.hpp src/object_state_impl.hpp
include/cgc1/cgc_internal_malloc_allocator.hpp
include/cgc1/cgc1.hpp
include/cgc1/concurrency.hpp
include/cgc1/declarations.hpp
include/cgc1/posix.hpp
include/cgc1/posix_slab.hpp
include/cgc1/posix_slab_impl.hpp
include/cgc1/win32_slab.hpp
include/cgc1/win32_slab_impl.hpp)
IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
target_link_libraries(cgc1 pthread)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
