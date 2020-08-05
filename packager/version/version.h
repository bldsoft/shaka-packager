// Copyright 2015 Google Inc. All rights reserved.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file or at
// https://developers.google.com/open-source/licenses/bsd

#include <string>

/**
 * SHAKA_VERSION_MAJOR:
 *
 * The major version of shaka at compile time:
 */
#define SHAKA_VERSION_MAJOR (2)
/**
 * SHAKA_VERSION_MINOR:
 *
 * The minor version of shaka at compile time:
 */
#define SHAKA_VERSION_MINOR (4)
/**
 * SHAKA_VERSION_MICRO:
 *
 * The micro version of shaka at compile time:
 */
#define SHAKA_VERSION_MICRO (3)
/**
 * SHAKA_VERSION_NANO:
 *
 * The nano version of shaka at compile time:
 * Actual releases have 0, GIT versions have 1, prerelease versions have 2-...
 */
#define SHAKA_VERSION_NANO (0)

/**
 * SHAKA_CHECK_VERSION:
 * @major: a number indicating the major version
 * @minor: a number indicating the minor version
 * @micro: a number indicating the micro version
 *
 * Check whether a shaka version equal to or greater than
 * major.minor.micro is present.
 */
#define SHAKA_CHECK_VERSION(major,minor,micro)	\
    (SHAKA_VERSION_MAJOR > (major) || \
     (SHAKA_VERSION_MAJOR == (major) && SHAKA_VERSION_MINOR > (minor)) || \
     (SHAKA_VERSION_MAJOR == (major) && SHAKA_VERSION_MINOR == (minor) && \
      SHAKA_VERSION_MICRO >= (micro)) || \
     (SHAKA_VERSION_MAJOR == (major) && SHAKA_VERSION_MINOR == (minor) && \
      SHAKA_VERSION_MICRO + 1 == (micro) && SHAKA_VERSION_NANO > 0))

namespace shaka {

/// @return URL of shaka-packager project.
std::string GetPackagerProjectUrl();

/// @return The version string.
std::string GetPackagerVersion();

/// Set version for testing.
/// @param version contains the injected testing version.
void SetPackagerVersionForTesting(const std::string& version);

}  // namespace shaka
