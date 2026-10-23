#include "ShapePointsEditor.h"

#include <algorithm>  // for clamp
#include <cmath>      // for abs, atan2, hypot, M_PI
#include <utility>    // for move

#include "control/tools/SnapToGridInputHandler.h"  // for SnapToGridInputHandler
#include "model/Element.h"                         // for Element, ELEMENT_STROKE
#include "model/Stroke.h"                          // for Stroke
#include "undo/ShapePointsUndoAction.h"            // for ShapePointsUndoAction

namespace {
/// Smallest and largest angle of an edited arc (radians)
constexpr double MIN_SWEEP = 0.01;
constexpr double MAX_SWEEP = 2 * M_PI - 1e-3;

/// Keeps the sign, with MIN_SWEEP <= |sweep| <= MAX_SWEEP
auto clampSweep(double sweep) -> double {
    double s = std::clamp(std::abs(sweep), MIN_SWEEP, MAX_SWEEP);
    return sweep < 0 ? -s : s;
}
}  // namespace

auto ShapePointsEditor::create(const InsertionOrder& elements) -> std::unique_ptr<ShapePointsEditor> {
    if (elements.size() != 1 || elements.front().e->getType() != ELEMENT_STROKE) {
        return nullptr;
    }
    auto* stroke = static_cast<Stroke*>(elements.front().e.get());
    auto shape = ShapePoints::recognize(*stroke);
    if (!shape) {
        return nullptr;
    }
    return std::make_unique<ShapePointsEditor>(stroke, std::move(*shape));
}

ShapePointsEditor::ShapePointsEditor(Stroke* stroke, ShapePoints::Shape shape): stroke(stroke), shape(std::move(shape)) {}

auto ShapePointsEditor::handleAt(double x, double y, double tolerance) const -> std::optional<size_t> {
    std::optional<size_t> best;
    double bestDist = tolerance;
    auto handles = ShapePoints::handles(shape);
    for (size_t i = 0; i < handles.size(); i++) {
        double d = std::hypot(handles[i].x - x, handles[i].y - y);
        if (d <= bestDist) {
            bestDist = d;
            best = i;
        }
    }
    return best;
}

void ShapePointsEditor::paintHandles(cairo_t* cr, double zoom, const GdkRGBA& color, double size) const {
    cairo_save(cr);
    cairo_set_dash(cr, nullptr, 0, 0);
    cairo_set_line_width(cr, 1.5);
    for (const Point& p: ShapePoints::handles(shape)) {
        cairo_new_path(cr);
        cairo_arc(cr, p.x * zoom, p.y * zoom, size / 2, 0, 2 * M_PI);
        cairo_set_source_rgb(cr, 1, 1, 1);
        cairo_fill_preserve(cr);
        gdk_cairo_set_source_rgba(cr, &color);
        cairo_stroke(cr);
    }
    cairo_restore(cr);
}

void ShapePointsEditor::startDrag(size_t handle) {
    dragged = handle;
    pointsBeforeDrag = stroke->getPointVector();
    if (shape.arc) {
        dragStartAngle = shape.arc->startAngle;
        dragSweep = shape.arc->sweep;
        dragLastAngle = handle == 0 ? dragStartAngle : dragStartAngle + dragSweep;
    }
}

void ShapePointsEditor::dragTo(const Point& p, bool alt, const SnapToGridInputHandler& snapping) {
    if (!dragged) {
        return;
    }
    if (shape.arc) {
        ShapePoints::Arc& arc = *shape.arc;
        // Follow the pointer around the circle continuously, so the arc goes the way the pointer went
        double angle = std::atan2(p.y - arc.center.y, p.x - arc.center.x);
        double delta = ShapePoints::angleDifference(angle, dragLastAngle);
        dragLastAngle += delta;
        if (*dragged == 0) {
            // Moving the start: the end stays in place
            const double end = dragStartAngle + dragSweep;
            dragStartAngle += delta;
            dragSweep -= delta;
            arc.sweep = clampSweep(snapping.snapAngle(dragSweep, alt));
            arc.startAngle = end - arc.sweep;
        } else {
            dragSweep += delta;
            arc.sweep = clampSweep(snapping.snapAngle(dragSweep, alt));
        }
    } else {
        Point snapped = snapping.snapToGrid(p, alt);
        shape.corners[*dragged] = Point(snapped.x, snapped.y);
    }
    stroke->setPointVector(ShapePoints::toPolyline(shape, stroke->getWidth()));
}

auto ShapePointsEditor::endDrag(const PageRef& page) -> UndoActionPtr {
    if (!dragged) {
        return nullptr;
    }
    dragged.reset();
    const auto& now = stroke->getPointVector();
    bool changed = now.size() != pointsBeforeDrag.size();
    for (size_t i = 0; !changed && i < now.size(); i++) {
        changed = now[i].x != pointsBeforeDrag[i].x || now[i].y != pointsBeforeDrag[i].y;
    }
    if (!changed) {
        return nullptr;
    }
    return std::make_unique<ShapePointsUndoAction>(page, stroke, std::move(pointsBeforeDrag), now);
}

auto ShapePointsEditor::getStroke() const -> Stroke* { return stroke; }
