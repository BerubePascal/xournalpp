#include "Arrowheads.h"

#include <algorithm>  // for min
#include <cmath>      // for atan2, cos, sin, M_PI
#include <iterator>   // for next

namespace {
/// Size of the head compared to the stroke width (same as the Arrow tool)
constexpr double HEAD_LENGTH_PER_THICKNESS = 7.0;
/// Maximal size of a head compared to the length of the polyline
constexpr double MAX_HEAD_LENGTH_RATIO = 0.4;
/// Half the opening angle of the head
constexpr double HEAD_HALF_ANGLE = M_PI / 6.0;

auto polylineLength(const std::vector<Point>& pts) -> double {
    double length = 0;
    for (size_t i = 1; i < pts.size(); i++) {
        length += pts[i - 1].lineLengthTo(pts[i]);
    }
    return length;
}

/**
 * Direction (angle) in which the polyline arrives at its tip. tip and inward iterate from the tip towards the inside.
 * We look at the point at distance headLength along the polyline, so a small wobble right at the tip of a curve does
 * not twist the head.
 */
template <typename It>
auto arrivalAngle(It tip, It end, double headLength) -> double {
    double walked = 0;
    It prev = tip;
    for (It it = std::next(tip); it != end; ++it) {
        walked += prev->lineLengthTo(*it);
        prev = it;
        if (walked >= headLength) {
            break;
        }
    }
    return std::atan2(tip->y - prev->y, tip->x - prev->x);
}

/// The 4 points to append after the tip: leg, tip, leg, tip
auto headPoints(const Point& tip, double angle, double headLength) -> std::vector<Point> {
    return {Point(tip.x - headLength * std::cos(angle + HEAD_HALF_ANGLE),
                  tip.y - headLength * std::sin(angle + HEAD_HALF_ANGLE)),
            tip,
            Point(tip.x - headLength * std::cos(angle - HEAD_HALF_ANGLE),
                  tip.y - headLength * std::sin(angle - HEAD_HALF_ANGLE)),
            tip};
}
}  // namespace

auto Arrowheads::shapeIndex(DrawingType drawingType) -> std::optional<size_t> {
    for (size_t i = 0; i < SHAPE_COUNT; i++) {
        if (SHAPES[i].drawingType == drawingType) {
            return i;
        }
    }
    return std::nullopt;
}

const char* Arrowheads::toString(ArrowheadMode mode) {
    switch (mode) {
        case ArrowheadMode::START:
            return "start";
        case ArrowheadMode::END:
            return "end";
        case ArrowheadMode::BOTH:
            return "both";
        default:
            return "none";
    }
}

auto Arrowheads::fromString(std::string_view s) -> ArrowheadMode {
    if (s == "start") {
        return ArrowheadMode::START;
    }
    if (s == "end") {
        return ArrowheadMode::END;
    }
    if (s == "both") {
        return ArrowheadMode::BOTH;
    }
    return ArrowheadMode::NONE;
}

auto Arrowheads::next(ArrowheadMode mode) -> ArrowheadMode {
    switch (mode) {
        case ArrowheadMode::NONE:
            return ArrowheadMode::END;
        case ArrowheadMode::END:
            return ArrowheadMode::START;
        case ArrowheadMode::START:
            return ArrowheadMode::BOTH;
        default:
            return ArrowheadMode::NONE;
    }
}

void Arrowheads::add(std::vector<Point>& points, ArrowheadMode mode, double thickness) {
    if (mode == ArrowheadMode::NONE || points.size() < 2) {
        return;
    }
    if (points.front().lineLengthTo(points.back()) == 0) {
        return;  // Closed shape: no ends to put arrowheads on
    }
    const double length = polylineLength(points);
    if (length <= 0) {
        return;
    }
    const double headLength = std::min(thickness * HEAD_LENGTH_PER_THICKNESS, length * MAX_HEAD_LENGTH_RATIO);

    const bool atStart = mode == ArrowheadMode::START || mode == ArrowheadMode::BOTH;
    const bool atEnd = mode == ArrowheadMode::END || mode == ArrowheadMode::BOTH;

    // Compute both directions before modifying the polyline
    const double startAngle = arrivalAngle(points.begin(), points.end(), headLength);
    const double endAngle = arrivalAngle(points.rbegin(), points.rend(), headLength);

    if (atEnd) {
        auto head = headPoints(points.back(), endAngle, headLength);
        points.insert(points.end(), head.begin(), head.end());
    }
    if (atStart) {
        // Prepend tip, leg, tip, leg: the polyline then goes back to its first point (the tip) and continues
        const Point tip = points.front();
        auto head = headPoints(tip, startAngle, headLength);
        head.pop_back();
        head.insert(head.begin(), tip);
        points.insert(points.begin(), head.begin(), head.end());
    }
}
