// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_burp
//

#pragma once

#include "../impl/version.h"

#include <daw/cpp_17.h>

#include <array>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace daw::burp {
	inline namespace DAW_BURP_VER {
		namespace concepts {
			namespace container_detect {
				template<typename T>
				using is_container_test = decltype( (void)( std::begin( std::declval<T &>( ) ) ),
				                                    (void)( std::end( std::declval<T &>( ) ) ),
				                                    (void)( std::size( std::declval<T &>( ) ) ),
				                                    (void)( std::declval<typename T::value_type>( ) ),
				                                    (void)( std::declval<T &>( ).insert(
				                                      std::end( std::declval<T &>( ) ),
				                                      std::declval<typename T::value_type>( ) ) ) );

				template<typename T>
				inline constexpr bool is_detected_container_v = daw::is_detected_v<is_container_test, T>;
			} // namespace container_detect

			/// @brief Concept to help deduce container types.
			template<typename T, typename = void>
			struct container_traits : std::bool_constant<container_detect::is_detected_container_v<T>> {};

			template<typename C>
			constexpr std::size_t container_size( C const &c ) {
				return std::size( c );
			}
		} // namespace concepts
	}   // namespace DAW_BURP_VER
} // namespace daw::burp
