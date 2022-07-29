// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_burp
//

#include "daw_burp_benchmark.h"

#include <daw/burp/daw_burp.h>
#include <daw/burp/daw_burp_describe.h>

#include <daw/daw_memory_mapped_file.h>
#include <daw/temp_file.h>

#include <boost/describe.hpp>
#include <cassert>
#include <cstdio>
#include <iostream>
#include <limits>
#include <memory>
#include <numeric>
#include <vector>

struct X {
	int x;
	X( ) = default;
	constexpr X( int i ) noexcept
	  : x( i ) {}
};
BOOST_DESCRIBE_STRUCT( X, ( ), ( x ) );

static std::vector<X> get_numbers( std::size_t count ) {
	auto result = std::vector<X>( count );
	std::iota( std::data( result ), daw::data_end( result ), 1 );
	return result;
}

static constexpr std::size_t NUM_RUNS = 10;

template<typename T>
static void do_bench( daw::burp::concepts::fd_t fd, T const &data ) {
	(void)daw::burp::benchmark::benchmark( NUM_RUNS,
	                                       sizeof( typename T::value_type ) * data.size( ),
	                                       "Writing to file",
	                                       [&] {
		                                       daw::do_not_optimize( data );
		                                       daw::do_not_optimize( data.data( ) );
		                                       ::lseek( fd.value, 0, SEEK_SET );
		                                       daw::burp::write( fd, data );
		                                       ::fsync( fd.value );
	                                       } );
}

template<typename T>
static void do_bench( daw::span<char> v, T const &data ) {
	(void)daw::burp::benchmark::benchmark( NUM_RUNS,
	                                       sizeof( typename T::value_type ) * data.size( ),
	                                       "Writing to buff",
	                                       [&] {
		                                       daw::do_not_optimize( data );
		                                       daw::do_not_optimize( data.data( ) );
		                                       auto s = v;
		                                       daw::burp::write( s, data );
	                                       } );
}

template<typename T>
static void do_bench( std::vector<char> &v, T const &data ) {
	(void)daw::burp::benchmark::benchmark( NUM_RUNS,
	                                       sizeof( typename T::value_type ) * data.size( ),
	                                       "Writing to buff",
	                                       [&] {
		                                       daw::do_not_optimize( data );
		                                       daw::do_not_optimize( data.data( ) );
		                                       v.clear( );
		                                       daw::burp::write( v, data );
	                                       } );
}

int main( ) {
#if not defined( NDEBUG )
	auto gb_data = get_numbers( 1000ULL * 1000ULL * 16ULL );
#else
	auto gb_data = get_numbers( 1000ULL * 1000ULL * 1000ULL );
#endif
	auto vec = std::vector<char>{ };
	vec.resize( gb_data.size( ) * sizeof( typename DAW_TYPEOF( gb_data )::value_type ) +
	            sizeof( size_t ) );
	std::cout << "Buffer\n";
	do_bench( daw::span<char>( vec.data( ), vec.size( ) ), gb_data );
	std::cout << "File via fd\n";
	auto tmp = daw::unique_temp_file{ };
	auto fname = tmp.native( );
	std::cout << "tmp file: " << fname << '\n';
	daw::burp::concepts::fd_t fd = tmp.secure_create_fd( );
	do_bench( fd, gb_data );
	::close( fd.value );
	auto buff =
	  daw::filesystem::memory_mapped_file_t<char>( fname, daw::filesystem::open_mode::read_write );

	std::cout << "File via memory map\n";
	do_bench( daw::span<char>( buff.data( ), buff.size( ) ), gb_data );
	std::cout << "done\n";
}
