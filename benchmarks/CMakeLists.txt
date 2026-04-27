add_executable(benchmark_spsc benchmark_spsc.cpp)
target_link_libraries(benchmark_spsc PRIVATE core_lib)

add_executable(benchmark_lob benchmark_lob.cpp)
target_link_libraries(benchmark_lob PRIVATE core_lib)
