/*
 * Xournal++
 *
 * Spacing of the lines / grid of a page background (the "r1" value of its configuration, see BackgroundConfig)
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <optional>  // for optional

#include "model/PageType.h"  // for PageType

namespace BackgroundSpacing {

/// The spacing of the background (in points), or nullopt if this kind of background has no spacing (e.g. plain)
std::optional<double> get(const PageType& pageType);

/// The same background with another spacing (in points)
PageType with(const PageType& pageType, double spacing);

}  // namespace BackgroundSpacing
