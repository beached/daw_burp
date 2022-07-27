// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_burp
//

#pragma once

namespace daw::burp {
	inline namespace DAW_BURP_VER {
		enum ErrorReason {
			None,
			OutputError,
		};

	} // namespace DAW_BURP_VER
} // namespace daw::burp

#define daw_burp_ensure( Bool, Reason ) \
	do {                                  \
		if( DAW_UNLIKELY( not( Bool ) ) ) { \
			throw Reason;                     \
		}                                   \
	} while( false )
