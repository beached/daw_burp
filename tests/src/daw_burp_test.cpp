// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_burp
//

#include <daw/burp/daw_burp.h>
#include <daw/burp/daw_burp_describe.h>

#include <boost/describe.hpp>
#include <cassert>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct X {
	int m1;
	int m2;
};
BOOST_DESCRIBE_STRUCT( X, ( ), ( m1, m2 ) );

struct Y {
	X m0;
	std::string m1;
};
BOOST_DESCRIBE_STRUCT( Y, ( ), ( m0, m1 ) );

struct Z {
	std::map<std::string, int> kv;
};
BOOST_DESCRIBE_STRUCT( Z, ( ), ( kv ) );

struct Foo {
	std::optional<Y> m0;
	std::vector<X> m1;
	std::shared_ptr<int> m2;
};
BOOST_DESCRIBE_STRUCT( Foo, ( ), ( m0, m1, m2 ) );

int main( ) {
	auto x0 = X{ 1, 2 };
	auto tp_x0 = daw::burp::generic_dto<X>::to_tuple( x0 );
	static_assert( std::tuple_size_v<decltype( tp_x0 )> == 2 );
	assert( ( std::get<0>( tp_x0 ) == 1 ) );
	assert( ( std::get<1>( tp_x0 ) == 2 ) );
	auto buff = std::string( );
	auto sz = daw::burp::write( buff, x0 );
	assert( buff.size( ) == sz );
	std::cout << "-----\n";
	for( char c : buff ) {
		std::cout << (int)c << '\n';
	}
	std::cout << "-----\n";
	buff.clear( );
	auto y0 = Y{ x0, "Hello World!!" };
	auto tp_y0 = daw::burp::generic_dto<Y>::to_tuple( y0 );
	static_assert( std::tuple_size_v<decltype( tp_y0 )> == 2 );
	auto const &tp_y0_x = std::get<0>( tp_y0 );
	assert( tp_y0_x.m1 == 1 );
	assert( tp_y0_x.m2 == 2 );
	assert( ( std::get<1>( tp_y0 ) == "Hello World!!" ) );
	sz = daw::burp::write( buff, y0 );
	std::cout << "-----\n";
	for( char c : buff ) {
		std::cout << (int)c << '\n';
	}
	std::cout << "-----\n";
}
