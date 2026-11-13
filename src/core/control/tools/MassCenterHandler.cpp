#include "MassCenterHandler.h"

#include <cmath>    // for M_PI, cos, sin, hypot, ceil
#include <memory>   // for make_unique
#include <utility>  // for move

#include "control/Control.h"                       // for Control
#include "control/tools/SnapToGridInputHandler.h"  // for SnapToGridInputHandler
#include "gui/XournalppCursor.h"                   // for XournalppCursor
#include "model/Document.h"                        // for Document
#include "model/Layer.h"                           // for Layer
#include "model/LineStyle.h"                       // for LineStyle
#include "model/Stroke.h"                          // for Stroke
#include "model/XojPage.h"                         // for XojPage
#include "undo/InsertUndoAction.h"                 // for InsertsUndoAction
#include "undo/UndoRedoHandler.h"                  // for UndoRedoHandler
#include "view/StrokeViewHelper.h"                 // for pathToCairo
#include "view/overlays/ShapeToolView.h"           // for ShapeToolView

/// Below this radius (in points) the symbol is not drawn (simple click without dragging)
constexpr double MIN_RADIUS = 0.5;

/// Fill value of a fully opaque filled stroke (see Stroke::setFill())
constexpr int OPAQUE_FILL = 255;

// Angles in radians, with y pointing down: 0 = right, π/2 = bottom, π = left, 3π/2 = top.
// Filled quadrants: top-left (π → 3π/2) and bottom-right (0 → π/2).

MassCenterHandler::MassCenterHandler(Control* control, const PageRef& page): BaseShapeHandler(control, page) {}

MassCenterHandler::~MassCenterHandler() = default;

void MassCenterHandler::onButtonPressEvent(const PositionInputData& pos, double zoom) {
    BaseShapeHandler::onButtonPressEvent(pos, zoom);
    // The preview view reads these when it is created (right after this call)
    this->stroke->setFill(OPAQUE_FILL);
    this->stroke->setLineStyle(LineStyle());
    this->center = this->startPoint;
    this->radius = 0;
}

auto MassCenterHandler::arc(double a0, double a1) const -> std::vector<Point> {
    // Same resolution as the ellipse tool
    auto n = static_cast<int>(std::ceil(5 + 0.6 * this->radius * std::abs(a1 - a0) / M_PI_2));
    std::vector<Point> pts;
    pts.reserve(static_cast<size_t>(n) + 1);
    for (int i = 0; i <= n; i++) {
        double a = a0 + (a1 - a0) * i / n;
        pts.emplace_back(center.x + radius * std::cos(a), center.y + radius * std::sin(a));
    }
    return pts;
}

auto MassCenterHandler::createShape(bool isAltDown, bool, bool) -> std::pair<std::vector<Point>, Range> {
    Point p = snappingHandler.snapToGrid(this->currPoint, isAltDown);
    this->radius = std::hypot(p.x - center.x, p.y - center.y);

    // The two filled quadrants as one closed path: center → left → top → center → right → bottom → center
    std::vector<Point> shape;
    shape.push_back(center);
    for (const Point& q: arc(M_PI, 1.5 * M_PI)) {
        shape.push_back(q);
    }
    shape.push_back(center);
    for (const Point& q: arc(0, M_PI_2)) {
        shape.push_back(q);
    }
    shape.push_back(center);

    Range rg(center.x - radius, center.y - radius);
    rg.addPoint(center.x + radius, center.y + radius);
    return {std::move(shape), rg};
}

void MassCenterHandler::addPreviewPath(cairo_t* cr) const {
    if (this->radius < MIN_RADIUS) {
        xoj::view::StrokeViewHelper::pathToCairo(cr, this->shape);
        return;
    }
    // The whole circle plus the two empty quadrants, filled with the even-odd rule:
    // the empty quadrants are covered twice (left empty), the filled ones once.
    // The edges drawn are the circle and the cross, as in the final strokes.
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);
    xoj::view::StrokeViewHelper::pathToCairo(cr, arc(0, 2 * M_PI));
    cairo_close_path(cr);
    for (auto [a0, a1]: {std::pair{1.5 * M_PI, 2 * M_PI}, std::pair{M_PI_2, M_PI}}) {
        std::vector<Point> slice;
        slice.push_back(center);
        for (const Point& q: arc(a0, a1)) {
            slice.push_back(q);
        }
        cairo_new_sub_path(cr);
        xoj::view::StrokeViewHelper::pathToCairo(cr, slice);
        cairo_close_path(cr);
    }
}

void MassCenterHandler::onButtonReleaseEvent(const PositionInputData& pos, double zoom) {
    control->getCursor()->activateDrawDirCursor(false);

    if (this->radius < MIN_RADIUS || this->shape.size() <= 1) {
        this->cancelStroke();
        return;
    }

    // The circle: same settings as the filled stroke, but not filled
    auto circle = this->stroke->cloneStroke();
    circle->setFill(-1);
    std::vector<Point> circlePts = arc(0, 2 * M_PI);
    circle->setPointVector(circlePts, &lastSnappingRange);

    this->stroke->setPointVector(this->shape, &lastSnappingRange);

    Range repaintRange = lastSnappingRange;
    repaintRange.addPadding(0.5 * this->stroke->getWidth());
    this->viewPool->dispatchAndClear(xoj::view::ShapeToolView::FINALIZATION_REQUEST, repaintRange);

    Layer* layer = page->getSelectedLayer();
    std::vector<const Element*> elements = {circle.get(), this->stroke.get()};
    control->getUndoRedoHandler()->addUndoAction(std::make_unique<InsertsUndoAction>(page, layer, elements));

    Document* doc = control->getDocument();
    doc->lock();
    layer->addElement(std::move(circle));
    layer->addElement(std::move(this->stroke));
    doc->unlock();
    page->fireElementsChanged(elements, repaintRange);

    control->getCursor()->updateCursor();
}
