/*
 * Xournal++
 *
 * Arrowheads that can be added to the open ends of shapes (line, spline, coordinate system, ...)
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <string_view>  // for string_view
#include <vector>       // for vector

#include "model/Point.h"  // for Point

enum class ArrowheadMode { NONE, START, END, BOTH };

namespace Arrowheads {

const char* toString(ArrowheadMode mode);

/// Unknown strings give ArrowheadMode::NONE
ArrowheadMode fromString(std::string_view s);

/// NONE -> END -> START -> BOTH -> NONE
ArrowheadMode next(ArrowheadMode mode);

/**
 * @brief Add arrowheads to the ends of a polyline, as requested by mode.
 *      The heads are drawn the same way as the Arrow tool: from the tip, along one leg, back to the tip, along the
 *      other leg and back to the tip. The head follows the direction of the polyline near its tip, so it also works
 *      on curves. Closed polylines (first point == last point) are left untouched.
 * @param points The polyline, modified in place
 * @param thickness The stroke width, used to size the heads
 */
void add(std::vector<Point>& points, ArrowheadMode mode, double thickness);

}  // namespace Arrowheads
