#include "ShapePoints.h"

#include <algorithm>  // for clamp
#include <cmath>      // for abs, atan2, ceil, cos, fmod, hypot, sin, sqrt, M_PI

#include "model/Stroke.h"  // for Stroke

namespace {
/// Tolerance (in document coordinates) for a point to lie on the segment between its neighbours. Strict: only points
/// put on a straight segment by a tool count (e.g. the straight parts of a spline), not points of a gentle curve
constexpr double COLLINEAR_TOLERANCE = 1e-3;
/// Tolerance (in document coordinates) for the points of an arc to lie on its circle
constexpr double ARC_TOLERANCE = 1e-3;
/// Fewest points of an arc drawn by the Arc tool (see arcPoints())
constexpr size_t MIN_ARC_POINTS = 9;

/// Length of the segments approximating an arc (in document coordinates)
constexpr double ARC_SEGMENT_LENGTH = 1.0;
constexpr size_t MIN_ARC_SEGMENTS = 8;
constexpr size_t MAX_ARC_SEGMENTS = 2000;

auto samePos(const Point& a, const Point& b) -> bool { return a.x == b.x && a.y == b.y; }

/// Arrowheads are drawn as tip, leg, tip, leg, tip (see Arrowheads::add())
auto hasHeadAtStart(const std::vector<Point>& p) -> bool {
    return p.size() >= 6 && samePos(p[0], p[2]) && samePos(p[0], p[4]) && !samePos(p[0], p[1]) &&
           !samePos(p[0], p[3]);
}
auto hasHeadAtEnd(const std::vector<Point>& p) -> bool {
    const size_t n = p.size();
    return n >= 6 && samePos(p[n - 1], p[n - 3]) && samePos(p[n - 1], p[n - 5]) && !samePos(p[n - 1], p[n - 2]) &&
           !samePos(p[n - 1], p[n - 4]);
}

/// Distance from p to the segment [a, b]
auto distanceToSegment(const Point& p, const Point& a, const Point& b) -> double {
    const double dx = b.x - a.x;
    const double dy = b.y - a.y;
    const double len2 = dx * dx + dy * dy;
    if (len2 == 0) {
        return std::hypot(p.x - a.x, p.y - a.y);
    }
    double t = ((p.x - a.x) * dx + (p.y - a.y) * dy) / len2;
    if (t < 0 || t > 1) {
        return COLLINEAR_TOLERANCE * 2;  // Not between a and b: this is a corner (e.g. a line going back)
    }
    return std::abs((p.x - a.x) * dy - (p.y - a.y) * dx) / std::sqrt(len2);
}

/// Removes repeated points and points lying on the segment between their neighbours
auto keepCorners(const std::vector<Point>& pts) -> std::vector<Point> {
    std::vector<Point> res;
    for (const Point& p: pts) {
        if (res.empty() || !samePos(res.back(), p)) {
            res.emplace_back(p.x, p.y);
        }
    }
    for (size_t i = 1; i + 1 < res.size();) {
        if (distanceToSegment(res[i], res[i - 1], res[i + 1]) < COLLINEAR_TOLERANCE) {
            res.erase(res.begin() + static_cast<std::ptrdiff_t>(i));
        } else {
            i++;
        }
    }
    return res;
}

/// The arc through the points, if they all lie on a circle (and do not close it)
auto recognizeArc(const std::vector<Point>& pts) -> std::optional<ShapePoints::Arc> {
    const size_t n = pts.size();
    if (n < MIN_ARC_POINTS || samePos(pts.front(), pts.back())) {
        return std::nullopt;
    }
    // Circle through the first, middle and last points
    const Point& a = pts.front();
    const Point& b = pts[n / 2];
    const Point& c = pts.back();
    const double d = 2 * (a.x * (b.y - c.y) + b.x * (c.y - a.y) + c.x * (a.y - b.y));
    if (std::abs(d) < 1e-9) {
        return std::nullopt;  // Aligned: not an arc
    }
    const double a2 = a.x * a.x + a.y * a.y;
    const double b2 = b.x * b.x + b.y * b.y;
    const double c2 = c.x * c.x + c.y * c.y;
    ShapePoints::Arc arc;
    arc.center = Point((a2 * (b.y - c.y) + b2 * (c.y - a.y) + c2 * (a.y - b.y)) / d,
                       (a2 * (c.x - b.x) + b2 * (a.x - c.x) + c2 * (b.x - a.x)) / d);
    arc.radius = a.lineLengthTo(arc.center);

    double previousAngle = std::atan2(a.y - arc.center.y, a.x - arc.center.x);
    arc.startAngle = previousAngle;
    for (const Point& p: pts) {
        if (std::abs(p.lineLengthTo(arc.center) - arc.radius) > ARC_TOLERANCE) {
            return std::nullopt;
        }
        double angle = std::atan2(p.y - arc.center.y, p.x - arc.center.x);
        arc.sweep += ShapePoints::angleDifference(angle, previousAngle);
        previousAngle = angle;
    }
    return arc;
}
}  // namespace

auto ShapePoints::angleDifference(double a, double b) -> double {
    double d = std::fmod(a - b, 2 * M_PI);
    if (d > M_PI) {
        d -= 2 * M_PI;
    } else if (d <= -M_PI) {
        d += 2 * M_PI;
    }
    return d;
}

auto ShapePoints::arcPoints(const Arc& arc) -> std::vector<Point> {
    auto segments = static_cast<size_t>(std::ceil(std::abs(arc.sweep) * arc.radius / ARC_SEGMENT_LENGTH));
    segments = std::clamp(segments, MIN_ARC_SEGMENTS, MAX_ARC_SEGMENTS);

    std::vector<Point> points;
    points.reserve(segments + 1);
    for (size_t i = 0; i <= segments; i++) {
        double a = arc.startAngle + arc.sweep * static_cast<double>(i) / static_cast<double>(segments);
        points.emplace_back(arc.center.x + arc.radius * std::cos(a), arc.center.y + arc.radius * std::sin(a));
    }
    return points;
}

auto ShapePoints::recognize(const Stroke& stroke) -> std::optional<Shape> {
    std::vector<Point> pts = stroke.getPointVector();
    for (const Point& p: pts) {
        if (p.z != Point::NO_PRESSURE) {
            return std::nullopt;  // Drawn by hand
        }
    }

    Shape shape;
    const bool atStart = hasHeadAtStart(pts);
    const bool atEnd = hasHeadAtEnd(pts);
    if (atEnd) {
        pts.resize(pts.size() - 4);
    }
    if (atStart) {
        pts.erase(pts.begin(), pts.begin() + 4);
    }
    shape.arrowheads = atStart && atEnd ? ArrowheadMode::BOTH :
                       atStart          ? ArrowheadMode::START :
                       atEnd            ? ArrowheadMode::END :
                                          ArrowheadMode::NONE;

    if ((shape.arc = recognizeArc(pts))) {
        return shape;
    }

    shape.corners = keepCorners(pts);
    if (shape.corners.size() >= 4 && samePos(shape.corners.front(), shape.corners.back())) {
        shape.closed = true;
        shape.corners.pop_back();
    }
    if (shape.corners.size() < 2 || shape.corners.size() > MAX_CORNERS) {
        return std::nullopt;
    }
    return shape;
}

auto ShapePoints::toPolyline(const Shape& shape, double thickness) -> std::vector<Point> {
    std::vector<Point> pts = shape.arc ? arcPoints(*shape.arc) : shape.corners;
    if (shape.closed) {
        pts.push_back(pts.front());
    } else {
        Arrowheads::add(pts, shape.arrowheads, thickness);
    }
    return pts;
}

auto ShapePoints::handles(const Shape& shape) -> std::vector<Point> {
    if (shape.arc) {
        const Arc& arc = *shape.arc;
        const double end = arc.startAngle + arc.sweep;
        return {Point(arc.center.x + arc.radius * std::cos(arc.startAngle),
                      arc.center.y + arc.radius * std::sin(arc.startAngle)),
                Point(arc.center.x + arc.radius * std::cos(end), arc.center.y + arc.radius * std::sin(end))};
    }
    return shape.corners;
}
