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
#include <daw/daw_cpp_feature_check.h>

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
				using is_contiguous_container_test =
				  decltype( (void)( std::data( std::declval<T &>( ) ) ),
				            (void)( std::size( std::declval<T &>( ) ) ) );

				template<typename T>
				inline constexpr bool is_detected_container_v = daw::is_detected_v<is_container_test, T>;

				template<typename T>
				using has_value_type_alias_test = typename T::value_type;

				template<typename T>
				inline constexpr bool has_value_type_alias_v =
				  daw::is_detected_v<has_value_type_alias_test, T>;

				template<typename T>
				inline constexpr bool is_character_type_v =
				  std::is_same_v<T, char> or std::is_same_v<T, unsigned char> or
				  std::is_same_v<T, signed char> or std::is_same_v<T, char16_t> or
				  std::is_same_v<T, char32_t>
#if defined( __cpp_char8_t )
#if __cpp_char8_t >= 201811L
				  or std::is_same_v<T, char>
#endif
#endif
				  ;

				template<typename T>
				inline constexpr bool is_fundamental_type_v =
				  std::is_arithmetic_v<T> or is_character_type_v<T> or std::is_same_v<T, bool>;

				template<typename, typename = void>
				struct container_value_type {
					static constexpr std::size_t size = 0;
					static constexpr std::size_t align = 0;
					static constexpr bool value = false;
					static constexpr bool is_fundamental_type = false;
				};

				template<typename T>
				struct container_value_type<T, std::enable_if_t<has_value_type_alias_v<T>>> {
					using type = typename T::value_type;
					static constexpr std::size_t size = sizeof( type );
					static constexpr std::size_t align = alignof( type );
					static constexpr bool value = true;
					static constexpr bool is_fundamental_type = is_fundamental_type_v<type>;
				};

				template<typename T, std::size_t N>
				struct container_value_type<T[N]> {
					using type = T;
					static constexpr std::size_t size = sizeof( type );
					static constexpr std::size_t align = alignof( type );
					static constexpr bool value = true;
					static constexpr bool is_fundamental_type = is_fundamental_type_v<type>;
				};

				template<typename T>
				inline constexpr bool has_byte_sized_value_type_v = container_value_type<T>::size == 1;

				template<typename T>
				inline constexpr bool is_fundamental_value_type_v =
				  container_value_type<T>::is_fundamental_type;
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
