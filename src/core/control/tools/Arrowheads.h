/*
 * Xournal++
 *
 * Arrowheads that can be added to the open ends of shapes (line, spline, coordinate system, ...)
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <cstddef>      // for size_t
#include <iterator>     // for size
#include <optional>     // for optional
#include <string_view>  // for string_view
#include <vector>       // for vector

#include "control/ToolEnums.h"  // for DrawingType
#include "enums/Action.enum.h"  // for Action
#include "model/Point.h"        // for Point

enum class ArrowheadMode { NONE, START, END, BOTH };

namespace Arrowheads {

/// The shapes with open ends, which can get arrowheads. Each one has its own setting and action.
struct Shape {
    DrawingType drawingType;
    Action action;            ///< Action whose state is the shape's arrowhead mode (as a string, see toString())
    const char* settingName;  ///< Name of the setting in the "tools" section of settings.xml
};
constexpr Shape SHAPES[] = {{DRAWING_TYPE_LINE, Action::TOOL_ARROWHEADS_LINE, "arrowheadsLine"},
                            {DRAWING_TYPE_SPLINE, Action::TOOL_ARROWHEADS_SPLINE, "arrowheadsSpline"},
                            {DRAWING_TYPE_COORDINATE_SYSTEM, Action::TOOL_ARROWHEADS_COORDINATE_SYSTEM,
                             "arrowheadsCoordinateSystem"}};
constexpr size_t SHAPE_COUNT = std::size(SHAPES);

/// Index of the shape in SHAPES, or nullopt if this drawing type does not support arrowheads
std::optional<size_t> shapeIndex(DrawingType drawingType);

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
