#include "ArcHandler.h"

#include <algorithm>  // for clamp, max, min
#include <cmath>      // for atan2, ceil, cos, sin, abs, M_PI

#include <gdk/gdkkeysyms.h>  // for GDK_KEY_Escape

#include "control/tools/ShapePoints.h"             // for ShapePoints
#include "control/tools/SnapToGridInputHandler.h"  // for SnapToGridInputHandler
#include "gui/inputdevices/InputEvents.h"          // for KeyEvent
#include "gui/inputdevices/PositionInputData.h"    // for PositionInputData
#include "model/Stroke.h"                          // for Stroke

namespace {
/// Smallest radius accepted (in document coordinates)
constexpr double MIN_RADIUS = 1.0;
/// The arc stays just short of a full circle
constexpr double MAX_SWEEP = 2 * M_PI - 1e-3;
}  // namespace

ArcHandler::ArcHandler(Control* control, const PageRef& page): BaseShapeHandler(control, page) {}

ArcHandler::~ArcHandler() = default;

void ArcHandler::onButtonPressEvent(const PositionInputData& pos, double zoom) {
    if (!this->stroke) {
        // 1st click: the center
        BaseShapeHandler::onButtonPressEvent(pos, zoom);
        this->center = this->startPoint;
        this->step = Step::CENTER_SET;
        return;
    }

    if (this->step == Step::CENTER_SET) {
        // 2nd click: radius and start of the arc
        Point p = snappingHandler.snapToGrid(Point(pos.x / zoom, pos.y / zoom), pos.isAltDown());
        double r = center.lineLengthTo(p);
        if (r < MIN_RADIUS) {
            return;  // Clicked on the center: wait for a proper click
        }
        this->radius = r;
        this->startAngle = std::atan2(p.y - center.y, p.x - center.x);
        this->lastAngle = this->startAngle;
        this->sweep = 0;
        this->step = Step::START_SET;
        return;
    }

    // 3rd click: the end of the arc follows the pointer until the button is released
    this->step = Step::END_DRAG;
    BaseShapeHandler::onMotionNotifyEvent(pos, zoom);
}

void ArcHandler::onButtonReleaseEvent(const PositionInputData& pos, double zoom) {
    if (this->step != Step::END_DRAG) {
        return;  // Not finished yet: the arc keeps following the pointer
    }
    BaseShapeHandler::onMotionNotifyEvent(pos, zoom);
    if (this->shape.size() <= 1) {
        cancel();
        return;
    }
    BaseShapeHandler::onButtonReleaseEvent(pos, zoom);  // Adds the stroke to the page
}

auto ArcHandler::onKeyReleaseEvent(const KeyEvent& event) -> bool {
    if (event.keyval == GDK_KEY_Escape) {
        cancel();
        return true;
    }
    return BaseShapeHandler::onKeyReleaseEvent(event);
}

void ArcHandler::cancel() {
    BaseShapeHandler::onSequenceCancelEvent();  // Clears the preview
    this->stroke.reset();                       // No stroke: the handler is done
}

auto ArcHandler::getArrowheadShape() const -> DrawingType {
    // While choosing the radius, the preview is a plain line from the center
    return this->step == Step::CENTER_SET ? DRAWING_TYPE_DEFAULT : DRAWING_TYPE_ARC;
}

auto ArcHandler::createShape(bool isAltDown, bool, bool) -> std::pair<std::vector<Point>, Range> {
    if (this->step == Step::CENTER_SET) {
        // Preview of the radius
        Point p = snappingHandler.snapToGrid(this->currPoint, isAltDown);
        Range rg(center.x, center.y);
        rg.addPoint(p.x, p.y);
        return {{center, p}, rg};
    }

    // Accumulate the angle swept by the pointer, so the arc goes the way the pointer went
    double angle = std::atan2(this->currPoint.y - center.y, this->currPoint.x - center.x);
    this->sweep = std::clamp(this->sweep + ShapePoints::angleDifference(angle, this->lastAngle), -MAX_SWEEP, MAX_SWEEP);
    this->lastAngle = angle;

    // Rotation snapping applies to the angle of the arc
    ShapePoints::Arc arc;
    arc.center = center;
    arc.radius = radius;
    arc.startAngle = startAngle;
    arc.sweep = std::clamp(snappingHandler.snapAngle(this->sweep, isAltDown), -MAX_SWEEP, MAX_SWEEP);

    std::pair<std::vector<Point>, Range> res;
    auto& [points, rg] = res;
    points = ShapePoints::arcPoints(arc);
    if (std::abs(arc.sweep) < 1e-6) {
        points.resize(1);  // No arc yet
    }
    rg = Range(points.front().x, points.front().y);
    for (const Point& p: points) {
        rg.addPoint(p.x, p.y);
    }
    return res;
}
