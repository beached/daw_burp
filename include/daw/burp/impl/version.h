// Copyright (c) Darrell Wright
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/beached/daw_burp
//

#pragma once

/// The version string used in namespace definitions.  Must be a valid namespace
/// name.
#if not defined( DAW_BURP_VER_OVERRIDE )
// Should be updated when a potential ABI break is anticipated
#define DAW_BURP_VER v0_0_1
#else
#define DAW_BURP_VER DAW_BURP_VER_OVERRIDE
#endif