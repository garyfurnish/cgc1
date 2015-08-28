IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
#add_compile_options(-fPIE)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
add_executable(cgc1_alloc_benchmark "main.cpp")
target_link_libraries(cgc1_alloc_benchmark cgc1)

add_executable(cgc1_alloc_benchmark_thread "main_thread.cpp")
target_link_libraries(cgc1_alloc_benchmark_thread cgc1)

IF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")
target_compile_options(cgc1_alloc_benchmark PUBLIC -fPIE)
ENDIF(NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows")


link_directories(${BOEHM_LIB})

add_executable(cgc1_alloc_benchmark_boehm "main.cpp")
target_include_directories(cgc1_alloc_benchmark_boehm PUBLIC ${BOEHM_INCLUDE})
target_compile_definitions(cgc1_alloc_benchmark_boehm PUBLIC BOEHM)
target_link_libraries(cgc1_alloc_benchmark_boehm gc pthread)

add_executable(cgc1_alloc_benchmark_thread_boehm "main_thread.cpp")
target_include_directories(cgc1_alloc_benchmark_thread_boehm PUBLIC ${BOEHM_INCLUDE})
target_compile_definitions(cgc1_alloc_benchmark_thread_boehm PUBLIC BOEHM)
target_link_libraries(cgc1_alloc_benchmark_thread_boehm gc pthread)