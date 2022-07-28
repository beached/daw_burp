// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_burp
//

#pragma once

#include "impl/version.h"

#include "concepts/daw_container_traits.h"
#include "concepts/daw_writable_output.h"

#include <daw/cpp_17.h>
#include <daw/daw_consteval.h>
#include <daw/daw_is_constant_evaluated.h>
#include <daw/daw_move.h>
#include <daw/daw_traits.h>

#include <cstddef>
#include <tuple>

namespace daw::burp {
	inline namespace DAW_BURP_VER {
		/// Types that use Boost.Describe need to specialize use_boost_describe_v for their type with a
		/// bool value of true.  This defaults to on, all described structs, but can be opted-out of if
		/// one wants to do a custom mapping of a Boost.Described struct
		template<typename, typename = void>
		inline constexpr bool use_boost_describe_v = true;

		template<typename T, typename = void>
		struct generic_dto;

		namespace burp_impl {
			template<typename T>
			using tuple_protocol_test = decltype( std::tuple_size_v<T> );

			template<typename T>
			using remove_rvalue_ref_t =
			  std::conditional_t<std::is_rvalue_reference_v<T>, daw::remove_cvref_t<T>, T>;

			template<typename Value, std::size_t... Is>
			constexpr auto to_tuple( Value &&value, std::index_sequence<Is...> ) {
				using result_t =
				  std::tuple<remove_rvalue_ref_t<decltype( std::get<Is>( DAW_FWD( value ) ) )>...>;
				return result_t{ std::get<Is>( DAW_FWD( value ) )... };
			}

		} // namespace burp_impl

		template<typename T>
		inline constexpr bool supports_tuple_protocol_v =
		  daw::is_detected_v<burp_impl::tuple_protocol_test, T> and not use_boost_describe_v<T>;

		/// @brief Tuple Protocol Types
		template<typename T>
		struct generic_dto<T, std::enable_if_t<supports_tuple_protocol_v<T>>> {
			static DAW_CONSTEVAL std::size_t member_count( ) {
				return std::tuple_size_v<T>;
			}

			template<typename Value>
			static constexpr auto to_tuple( Value &&value ) {
				return burp_impl::to_tuple( DAW_FWD( value ),
				                            std::make_index_sequence<member_count( )>{ } );
			}
		};

		template<typename Writable, typename T>
		std::size_t write( Writable &write_out, T const &value );

		namespace burp_impl {
			template<typename T>
			using has_generic_dto_test = decltype( generic_dto<T>{ } );

			template<typename T>
			inline constexpr bool has_generic_dto_v = daw::is_detected_v<has_generic_dto_test, T>;

			template<typename T, std::size_t... Is>
			DAW_CONSTEVAL bool
			is_class_of_fundamental_types_without_padding_impl( std::index_sequence<Is...> ) {
				using dto = generic_dto<T>;
				using tp_t = DAW_TYPEOF( dto::to_tuple( std::declval<T &>( ) ) );
				if constexpr( sizeof( T ) != alignof( T ) ) {
					return false;
				} else if constexpr( ( sizeof( daw::remove_cvref_t<std::tuple_element_t<Is, tp_t>> ) +
				                       ... ) != sizeof( T ) ) {
					return false;
				} else if constexpr( not( concepts::container_detect::is_fundamental_type_v<
				                            daw::remove_cvref_t<std::tuple_element_t<Is, tp_t>>> and
				                          ... ) ) {
					return false;
				} else {
					return true;
				}
			}

			template<typename T>
			inline constexpr bool is_class_of_fundamental_types_without_padding_v = [] {
				if constexpr( has_generic_dto_v<T> ) {
					using dto = generic_dto<T>;
					return is_class_of_fundamental_types_without_padding_impl<T>(
					  std::make_index_sequence<dto::member_count( )>{ } );
				}
				return false;
			}( );

			template<typename Writable, typename T, std::size_t... Is>
			std::size_t write_impl( Writable &writable, T const &value, std::index_sequence<Is...> ) {
				using dto = generic_dto<T>;
				using out_t = concepts::writable_output_trait<Writable>;
				auto result = std::size_t{ 0 };
				auto const tp = dto::to_tuple( value );
				if constexpr( is_class_of_fundamental_types_without_padding_v<T> ) {
					result += sizeof( T );
					out_t::write( writable,
					              daw::span( reinterpret_cast<char const *>( &value ), sizeof( T ) ) );
					return result;
				}
				auto const do_write = [&]( auto const &v ) {
					using current_type = DAW_TYPEOF( v );
					if constexpr( has_generic_dto_v<current_type> or
					              concepts::is_container_v<current_type> ) {
						result += daw::burp::write( writable, v );
					} else {
						static_assert( concepts::container_detect::is_fundamental_type_v<current_type>,
						               "Type is not a fundamental type or is not mapped" );
						result += sizeof( current_type );
						out_t::write(
						  writable,
						  daw::span( reinterpret_cast<char const *>( &v ), sizeof( current_type ) ) );
					}
					return true;
				};
				bool expander[]{ do_write( std::get<Is>( tp ) )... };
				(void)expander;
				return result;
			}

			template<typename Tp, std::size_t... Is>
			DAW_CONSTEVAL std::size_t total_member_size( std::index_sequence<Is...> ) {
				return ( sizeof( std::tuple_element_t<Is, Tp> ) + ... );
			}

			template<typename T>
			inline constexpr bool is_contiguous_array_of_fundamental_like_types_v = [] {
				if constexpr( not concepts::is_contiguous_container_v<T> ) {
					return false;
				} else if constexpr( concepts::container_detect::is_fundamental_value_type_v<T> ) {
					return true;
				} else {
					using element_t = DAW_TYPEOF( *std::begin( std::declval<T &>( ) ) );
					if constexpr( not std::is_trivially_copyable_v<element_t> ) {
						return false;
					} else {
						return burp_impl::is_class_of_fundamental_types_without_padding_v<element_t>;
					}
				}
			}( );
		} // namespace burp_impl

		template<typename Writable, typename T>
		std::size_t write( Writable &writable, T const &value ) {
			static_assert( concepts::is_writable_output_type_v<Writable>,
			               "Writable does not fulfill the writable output concept." );
			using out_t = concepts::writable_output_trait<Writable>;
			if constexpr( burp_impl::has_generic_dto_v<T> ) {
				using dto = generic_dto<T>;
				return burp_impl::write_impl( writable,
				                              value,
				                              std::make_index_sequence<dto::member_count( )>{ } );
			} else if constexpr( burp_impl::is_contiguous_array_of_fundamental_like_types_v<T> ) {
				// String like types
				auto const sz = concepts::container_size( value );
				out_t::write( writable, daw::span( reinterpret_cast<char const *>( &sz ), sizeof( sz ) ) );
				auto const count = std::size( value );
				out_t::write(
				  writable,
				  daw::span( reinterpret_cast<char const *>( std::data( value ) ),
				             count * concepts::container_detect::container_value_type<T>::size ) );
				return sizeof( sz ) + ( sizeof( T ) * count );
			} else if constexpr( concepts::is_container_v<T> ) {
				auto const sz = concepts::container_size( value );
				out_t::write( writable, daw::span( reinterpret_cast<char const *>( &sz ), sizeof( sz ) ) );
				auto result = sizeof( sz );
				using value_type = typename T::value_type;
				for( auto const &element : value ) {
					result += burp::write( writable, element );
				}
				return result;
			} else {
				static_assert( concepts::container_detect::is_fundamental_type_v<T>,
				               "Could not find mapping for type and it isn't a fundamental type" );
				out_t::write( writable,
				              daw::span( reinterpret_cast<char const *>( &value ), sizeof( T ) ) );
				return sizeof( T );
			}
		}
	} // namespace DAW_BURP_VER
} // namespace daw::burp