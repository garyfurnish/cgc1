find_library(SystemGC NAMES gc PATHS ${BOEHM_LIB}  NO_DEFAULT_PATH )
IF(SystemGC)
add_executable(cgc1_alloc_benchmark_boehm "main.cpp")
target_include_directories(cgc1_alloc_benchmark_boehm PUBLIC ${BOEHM_INCLUDE})
target_compile_definitions(cgc1_alloc_benchmark_boehm PUBLIC BOEHM)
target_link_libraries(cgc1_alloc_benchmark_boehm  ${SystemGC} pthread)

add_executable(cgc1_alloc_benchmark_thread_boehm "main_thread.cpp")
target_include_directories(cgc1_alloc_benchmark_thread_boehm PUBLIC ${BOEHM_INCLUDE})
target_compile_definitions(cgc1_alloc_benchmark_thread_boehm PUBLIC BOEHM)
target_link_libraries(cgc1_alloc_benchmark_thread_boehm ${SystemGC} pthread)
ENDIF(SystemGC)


include_directories(../mcppalloc/mcppalloc/include)
include_directories(../mcppalloc_utils/mcppalloc_utils/include)
include_directories(../mcppalloc_sparse/mcppalloc_sparse/include/)
include_directories(../mcppalloc_bitmap/mcppalloc_bitmap/include)
include_directories(../mcppalloc_slab_allocator/mcppalloc_slab_allocator/include)
include_directories(../mcppalloc_bitmap_allocator/mcppalloc_bitmap_allocator/include/)
include_directories(../cgc1/include)

IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_executable(cgc1_alloc_benchmark "main.cpp")
target_include_directories(cgc1_alloc_benchmark PUBLIC "../cgc1/include")
target_link_libraries(cgc1_alloc_benchmark cgc1)

add_executable(cgc1_alloc_benchmark_sparse "main.cpp")
target_include_directories(cgc1_alloc_benchmark_sparse PUBLIC "../cgc1/include")
target_link_libraries(cgc1_alloc_benchmark_sparse cgc1)
target_compile_definitions(cgc1_alloc_benchmark_sparse PUBLIC CGC1_SPARSE)

add_executable(cgc1_alloc_benchmark_thread "main_thread.cpp")
target_include_directories(cgc1_alloc_benchmark_thread PUBLIC "../cgc1/include")
target_link_libraries(cgc1_alloc_benchmark_thread cgc1)

add_executable(cgc1_alloc_benchmark_thread_sparse "main_thread.cpp")
target_include_directories(cgc1_alloc_benchmark_thread_sparse PUBLIC "../cgc1/include")
target_link_libraries(cgc1_alloc_benchmark_thread_sparse cgc1)
target_compile_definitions(cgc1_alloc_benchmark_thread_sparse PUBLIC CGC1_SPARSE)

IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
target_compile_options(cgc1_alloc_benchmark PUBLIC -fPIE)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")

