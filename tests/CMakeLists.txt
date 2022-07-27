# Copyright (c) Darrell Wright
#
# Distributed under the Boost Software License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# Official repository: https://github.com/beached/daw_burp
#

add_library( daw_burp_test_lib INTERFACE )
target_link_libraries( daw_burp_test_lib INTERFACE daw::daw-burp )
target_compile_options( daw_burp_test_lib INTERFACE $<$<CXX_COMPILER_ID:MSVC>:/permissive-> )

add_executable( daw_burp_bin src/daw_burp_test.cpp )
target_link_libraries( daw_burp_bin PRIVATE daw_burp_test_lib )
add_test( NAME daw_burp_test COMMAND daw_burp_bin )

add_executable( daw_burp_array_bench_bin src/daw_burp_array_bench.cpp )
target_link_libraries( daw_burp_array_bench_bin PRIVATE daw_burp_test_lib )
add_test( NAME daw_burp_array_bench_test COMMAND daw_burp_array_bench_bin )
