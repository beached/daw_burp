// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_burp
//

#pragma once

#include "../impl/errors.h"
#include "../impl/version.h"

#include "daw_writable_output_fwd.h"

#include <daw/daw_algorithm.h>
#include <daw/daw_character_traits.h>
#include <daw/daw_consteval.h>
#include <daw/daw_likely.h>
#include <daw/daw_span.h>

#include <cstdio>
#include <iostream>
#include <string>

#if __has_include( <unistd.h> )
#include <unistd.h>
#define DAW_HAS_UNISTD
#endif

namespace daw::burp {
	inline namespace DAW_BURP_VER {
		namespace concepts {
			namespace writeable_output_details {
				template<typename T, typename B>
				constexpr T *copy_to_buffer( T *buff, daw::span<B const> source ) {
#if defined( DAW_IS_CONSTANT_EVALUATED )
					if( DAW_IS_CONSTANT_EVALUATED( ) ) {
#endif
						daw::algorithm::transform_n( source.data( ), buff, source.size( ), []( auto c ) {
							return static_cast<T>( c );
						} );
#if defined( DAW_IS_CONSTANT_EVALUATED )
					} else {
						memcpy( buff, source.data( ), source.size( ) );
					}
#endif
					return buff + source.size( );
				}

				template<typename T>
				inline constexpr bool is_byte_type_v =
				  std::is_same_v<T, std::byte> or std::is_same_v<T, unsigned char>;

				template<typename T>
				using is_char_sized = std::bool_constant<sizeof( T ) == 1>;

				template<typename T>
				inline constexpr bool is_char_sized_character_v = false;

				template<>
				inline constexpr bool is_char_sized_character_v<char> = true;

#if defined( __cpp_lib_char8_t )
#if __cpp_lib_char8_t >= 201907L
				template<>
				inline constexpr bool is_char_sized_character_v<char8_t> = true;
#endif
#endif
			} // namespace writeable_output_details

			/// @brief Specialization for character pointer
			template<typename T>
			struct writable_output_trait<
			  T *,
			  std::enable_if_t<( writeable_output_details::is_char_sized_character_v<T> or
			                     writeable_output_details::is_byte_type_v<T> )>> : std::true_type {

				static constexpr std::size_t capacity( T const * ) noexcept {
					return std::numeric_limits<std::size_t>::max( );
				}

				template<typename... ContiguousBytes>
				static constexpr void write( T *&ptr, ContiguousBytes... blobs ) {
					static_assert( sizeof...( ContiguousBytes ) > 0 );
					daw_burp_ensure( ptr, daw::burp::ErrorReason::OutputError );
					constexpr auto writer = []( T *&p, auto sv ) {
						if( DAW_UNLIKELY( sv.empty( ) ) ) {
							return 0;
						}
						p = writeable_output_details::copy_to_buffer( p, sv );
						return 0;
					};
					(void)( writer( ptr, blobs ) | ... );
				}

				static constexpr void put( T *&ptr, char c ) {
					daw_burp_ensure( ptr, daw::burp::ErrorReason::OutputError );
					*ptr = static_cast<T>( c );
					++ptr;
				}
			};

			/// @brief Specialization for ostream &
			template<typename T>
			struct writable_output_trait<T, std::enable_if_t<std::is_base_of_v<std::ostream, T>>> :
			  std::true_type {

				static constexpr std::size_t capacity( std::ostream const & ) noexcept {
					return std::numeric_limits<std::size_t>::max( );
				}

				template<typename... ContiguousBytes>
				static inline void write( std::ostream &os, ContiguousBytes... blobs ) {
					static_assert( sizeof...( ContiguousBytes ) > 0 );
					constexpr auto writer = []( std::ostream &o, auto sv ) {
						if( sv.empty( ) ) {
							return 0;
						}
						o.write( sv.data( ), static_cast<std::streamsize>( sv.size( ) ) );
						daw_burp_ensure( static_cast<bool>( o ), daw::burp::ErrorReason::OutputError );
						return 0;
					};
					(void)( writer( os, blobs ) | ... );
				}

				static inline void put( std::ostream &os, char c ) {
					os.put( c );
					daw_burp_ensure( static_cast<bool>( os ), daw::burp::ErrorReason::OutputError );
				}
			};

			/// @brief Specialization for FILE * streams
			template<>
			struct writable_output_trait<std::FILE *> : std::true_type {

				static constexpr std::size_t capacity( std::FILE const * ) noexcept {
					return std::numeric_limits<std::size_t>::max( );
				}

				template<typename... ContiguousBytes>
				static inline void write( std::FILE *fp, ContiguousBytes... blobs ) {
					static_assert( sizeof...( ContiguousBytes ) > 0 );
					constexpr auto writer = []( std::FILE *f, auto sv ) {
						if( sv.empty( ) ) {
							return 0;
						}
						auto ret = std::fwrite( sv.data( ), 1, sv.size( ), f );
						daw_burp_ensure( ret == sv.size( ), daw::burp::ErrorReason::OutputError );
						return 0;
					};
					(void)( writer( fp, blobs ) | ... );
				}

				static inline void put( std::FILE *f, char c ) {
					auto ret = std::fputc( c, f );
					daw_burp_ensure( ret == c, daw::burp::ErrorReason::OutputError );
				}
			};

#if defined( DAW_HAS_UNISTD )
			struct fd_t {
				int value;

				constexpr fd_t( int fd ) noexcept
				  : value( fd ) {}
			};

			template<>
			struct writable_output_trait<fd_t> : std::true_type {
				static constexpr std::size_t max_chunk_size = 32'768ULL;

				static constexpr std::size_t capacity( fd_t const & ) noexcept {
					return std::numeric_limits<std::size_t>::max( );
				}

				template<typename... ContiguousBytes>
				static inline void write( fd_t fd, ContiguousBytes... blobs ) {
					static_assert( sizeof...( ContiguousBytes ) > 0 );
					constexpr auto writer = []( fd_t f, auto blob ) {
						auto const *b_ptr = std::data( blob );
						std::size_t b_sz = std::size( blob );
						while( b_sz > 0 ) {
							auto const chunk = b_sz <= max_chunk_size ? b_sz : max_chunk_size;
							auto ret = ::write( f.value, b_ptr, chunk );
							daw_burp_ensure( ret == chunk, daw::burp::ErrorReason::OutputError );
							b_sz -= chunk;
							b_ptr += chunk;
						}
						return 0;
					};
					(void)( writer( fd, blobs ) | ... );
				}

				static inline void put( fd_t fd, char c ) {
					return write( fd, daw::span<char>( &c, 1 ) );
				}
			};
#endif

			namespace writeable_output_details {
				template<typename T, typename CharT>
				using span_like_range_test =
				  decltype( (void)( std::declval<T &>( ).subspan( 1 ) ),
				            (void)( std::declval<std::size_t &>( ) = std::declval<T &>( ).size( ) ),
				            (void)( std::declval<bool &>( ) = std::declval<T &>( ).empty( ) ),
				            (void)( *std::declval<T &>( ).data( ) = std::declval<CharT>( ) ) );
				template<typename T, typename CharT>
				inline constexpr bool is_span_like_range_v =
				  daw::is_detected_v<span_like_range_test, T, CharT> and
				  ( writeable_output_details::is_char_sized_character_v<CharT> or
				    writeable_output_details::is_byte_type_v<CharT> );
			} // namespace writeable_output_details

			/// @brief Specialization for a span to a buffer with a fixed size
			template<typename T>
			struct writable_output_trait<
			  T,
			  std::enable_if_t<
			    writeable_output_details::is_span_like_range_v<T, typename T::value_type>>> :
			  std::true_type {
				using CharT = typename T::value_type;

				static constexpr std::size_t capacity( T const &s ) noexcept {
					return std::size( s );
				}

				template<typename... ContiguousBytes>
				static constexpr void write( T &out, ContiguousBytes... blobs ) {
					static_assert( sizeof...( ContiguousBytes ) > 0 );
					auto const total_size = ( std::size( blobs ) + ... );
					auto *ptr = out.data( );
					auto const writer = [&]( auto blob ) {
						(void)writeable_output_details::copy_to_buffer( ptr, blob );
						ptr += std::size( blob );
						return 0;
					};
					(void)( writer( blobs ) | ... );
					out = out.subspan( ptr - out.data( ) );
				}

				static constexpr void put( T &out, char c ) {
					daw_burp_ensure( not out.empty( ), daw::burp::ErrorReason::OutputError );
					*out.data( ) = static_cast<CharT>( c );
					out.remove_prefix( 1 );
				}
			};

			namespace writeable_output_details {
				template<typename T, typename CharT>
				using resizable_contiguous_range_test =
				  decltype( (void)( std::declval<T &>( ).resize( std::size_t{ 0 } ) ),
				            (void)( std::declval<T &>( ).size( ) ),
				            (void)( *std::declval<T &>( ).data( ) ),
				            (void)( *std::declval<T &>( ).data( ) = std::declval<CharT>( ) ),
				            (void)( std::declval<T &>( ).push_back( std::declval<CharT>( ) ) ),
				            (void)( static_cast<CharT>( 'a' ) ) );

				template<typename Container, typename CharT>
				inline constexpr bool is_resizable_contiguous_range_v =
				  daw::is_detected_v<resizable_contiguous_range_test, Container, CharT>;

				template<typename Container, typename CharT>
				inline constexpr bool is_string_like_writable_output_v =
				  (writeable_output_details::is_char_sized_character_v<CharT> or
				   writeable_output_details::is_byte_type_v<
				     CharT>)and writeable_output_details::is_resizable_contiguous_range_v<Container, CharT>;
			} // namespace writeable_output_details

			/// @brief Specialization for a resizable continain like vector/string
			template<typename Container>
			struct writable_output_trait<
			  Container,
			  std::enable_if_t<writeable_output_details::is_string_like_writable_output_v<
			    Container,
			    typename Container::value_type>>> : std::true_type {
				using CharT = typename Container::value_type;

				static constexpr std::size_t capacity( Container const & ) noexcept {
					return std::numeric_limits<std::size_t>::max( );
				}

				template<typename... ContiguousBytes>
				static inline void write( Container &out, ContiguousBytes... blobs ) {
					static_assert( sizeof...( ContiguousBytes ) > 0 );
					auto const start_pos = out.size( );
					auto const total_size = ( std::size( blobs ) + ... );
					out.resize( start_pos + total_size );

					constexpr auto writer = []( CharT *&p, auto sv ) {
						if( sv.empty( ) ) {
							return 0;
						}
						p = writeable_output_details::copy_to_buffer( p, sv );
						return 0;
					};
					auto *ptr = out.data( ) + start_pos;
					(void)( writer( ptr, blobs ) | ... );
				}

				static inline void put( Container &out, char c ) {
					out.push_back( static_cast<CharT>( c ) );
				}
			};

			namespace writeable_output_details {
				template<typename T>
				using is_writable_output_iterator_test =
				  decltype( *std::declval<T &>( ) = 'c', ++std::declval<T &>( ) );

				template<typename T>
				inline constexpr bool is_writable_output_iterator_v =
				  not std::is_pointer_v<T> and daw::is_detected_v<is_writable_output_iterator_test, T>;
			} // namespace writeable_output_details

			/// @brief Specialization for output iterators
			template<typename T>
			struct writable_output_trait<
			  T,
			  std::enable_if_t<writeable_output_details::is_writable_output_iterator_v<T>>> :
			  std::true_type {

				static constexpr std::size_t capacity( T const & ) noexcept {
					return std::numeric_limits<std::size_t>::max( );
				}

				template<typename... ContiguousBytes>
				static constexpr void write( T &it, ContiguousBytes... blobs ) {
					static_assert( sizeof...( ContiguousBytes ) > 0 );
					constexpr auto writer = []( T &i, auto b ) {
						for( auto c : daw::span( b ) ) {
							*i = static_cast<char>( c );
							++i;
						}
						return 0;
					};
					(void)( writer( it, blobs ) | ... );
				}

				static constexpr void put( T &it, char c ) {
					*it = c;
					++it;
				}
			};
		} // namespace concepts
	}   // namespace DAW_BURP_VER
} // namespace daw::burp
