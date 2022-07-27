// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_burp
//

#include <daw/burp/daw_burp.h>
#include <daw/burp/daw_burp_describe.h>

#include <daw/daw_benchmark.h>

#include <boost/describe.hpp>
#include <cassert>
#include <cstdio>
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

static constexpr std::size_t NUM_RUNS = 2;

static void do_bench( FILE *fs, std::vector<int> const &data ) {
	(void)daw::bench_n_test_mbs<NUM_RUNS>( "Writing to file", sizeof( int ) * data.size( ), [&] {
		daw::do_not_optimize( data );
		daw::do_not_optimize( data.data( ) );
		std::rewind( fs );
		daw::burp::write( fs, data );
	} );
}

template<typename T>
static void do_bench( daw::span<char> v, T const &data ) {
	(void)daw::bench_n_test_mbs<NUM_RUNS>( "Writing to buff",
	                                       sizeof( typename T::value_type ) * data.size( ),
	                                       [&] {
		                                       daw::do_not_optimize( data );
		                                       daw::do_not_optimize( data.data( ) );
		                                       auto s = v;
		                                       daw::burp::write( s, data );
	                                       } );
}

int main( ) {
	auto gb_data = get_numbers( 1000ULL * 1000ULL * 1ULL );
	// FILE *fs = std::fopen( "/tmp/burp_bench_out.bin", "w" );
	// assert( fs );
	auto buff = std::vector<char>( );
	buff.resize( gb_data.size( ) * sizeof( typename decltype( gb_data )::value_type ) * 2 );
	do_bench( daw::span<char>( buff ), gb_data );
	// std::fclose( fs );
}
