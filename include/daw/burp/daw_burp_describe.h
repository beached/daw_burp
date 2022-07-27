// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_burp
//

#pragma once

#include "daw_burp.h"
#include "impl/version.h"

#include <boost/describe.hpp>
#include <boost/mp11.hpp>
#include <cstddef>
#include <tuple>
#include <utility>

namespace daw::burp {
	inline namespace DAW_BURP_VER {

		namespace describe_impl {
			template<typename>
			inline constexpr std::nullptr_t member_list_size_v = nullptr;

			template<template<typename...> typename List, typename... Ts>
			inline constexpr std::size_t member_list_size_v<List<Ts...>> = sizeof...( Ts );
		} // namespace describe_impl

		template<typename T>
		struct generic_dto<T,
		                   std::enable_if_t<boost::describe::has_describe_members<T>::value and
		                                    use_boost_describe_v<T>>> {
		private:
			using pub_desc_t = boost::describe::describe_members<T, boost::describe::mod_public>;
			using pri_desc_t = boost::describe::describe_members<T, boost::describe::mod_private>;
			using pro_desc_t = boost::describe::describe_members<T, boost::describe::mod_protected>;
			static_assert(
			  boost::mp11::mp_empty<pri_desc_t>::value,
			  "Classes with private member variables are not supported. Must use a manual mapping." );
			static_assert(
			  boost::mp11::mp_empty<pro_desc_t>::value,
			  "Classes with protected member variables are not supported. Must use a manual mapping." );

			template<template<typename...> typename List, typename... Ts>
			static inline constexpr auto to_tuple_impl( T const &value, List<Ts...> const & ) noexcept {
				return std::forward_as_tuple( value.*Ts::pointer... );
			}

		public:
			static DAW_CONSTEVAL std::size_t member_count( ) {
				return describe_impl::member_list_size_v<pub_desc_t>;
			}

			static constexpr auto to_tuple( T const &value ) noexcept {
				return to_tuple_impl( value, pub_desc_t{ } );
			}
		};
	} // namespace DAW_BURP_VER
} // namespace daw::burp