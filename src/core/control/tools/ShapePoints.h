/*
 * Xournal++
 *
 * Recognizes simple shapes stored as strokes, so they can be edited and the stroke rebuilt:
 *  - shapes made of straight segments (lines, rectangles, coordinate systems, polygons...): their corners
 *  - circular arcs (see ArcHandler): their center, radius and angles
 * including their arrowheads.
 *
 * Shapes are stored as plain polylines, so this works from the points only: arrowheads are recognized from their
 * point pattern (see Arrowheads::add()), arcs from all their points lying on a circle, and points lying exactly on
 * a straight segment between their neighbours are not corners.
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <cstddef>   // for size_t
#include <optional>  // for optional
#include <vector>    // for vector

#include "control/tools/Arrowheads.h"  // for ArrowheadMode
#include "model/Point.h"               // for Point

class Stroke;

namespace ShapePoints {

/// Strokes with more corners than this are not considered simple shapes (e.g. curves, freehand strokes)
constexpr size_t MAX_CORNERS = 32;

struct Arc {
    Point center;
    double radius = 0;
    double startAngle = 0;  ///< radians
    double sweep = 0;       ///< signed angle from the start to the end (radians)
};

struct Shape {
    /// Shapes made of straight segments. For closed shapes, the closing point (equal to the first one) is not repeated
    std::vector<Point> corners;
    bool closed = false;
    /// Set for circular arcs (corners is then unused)
    std::optional<Arc> arc;
    ArrowheadMode arrowheads = ArrowheadMode::NONE;
};

/// The shape of the stroke, or nullopt if it is not a simple shape
std::optional<Shape> recognize(const Stroke& stroke);

/// The polyline of the shape, with its arrowheads
std::vector<Point> toPolyline(const Shape& shape, double thickness);

/// Where the shape can be edited: its corners, or the start and end of an arc
std::vector<Point> handles(const Shape& shape);

/// The polyline approximating an arc (without arrowheads)
std::vector<Point> arcPoints(const Arc& arc);

/// a - b, brought into (-pi, pi]
double angleDifference(double a, double b);

}  // namespace ShapePoints
