#include "ShapePointsUndoAction.h"

#include <utility>  // for move

#include "control/Control.h"  // for Control
#include "model/Document.h"   // for Document
#include "model/Stroke.h"     // for Stroke
#include "model/XojPage.h"    // for XojPage
#include "util/Range.h"       // for Range
#include "util/i18n.h"        // for _

ShapePointsUndoAction::ShapePointsUndoAction(const PageRef& page, Stroke* stroke, std::vector<Point> before,
                                             std::vector<Point> after):
        UndoAction("ShapePointsUndoAction"), stroke(stroke), before(std::move(before)), after(std::move(after)) {
    this->page = page;
}

void ShapePointsUndoAction::apply(Control* control, const std::vector<Point>& points) {
    Range rg(stroke->getBoundingBox());
    Document* doc = control->getDocument();
    doc->lock();
    stroke->setPointVector(points);
    doc->unlock();
    rg = rg.unite(Range(stroke->getBoundingBox()));
    this->page->fireRangeChanged(rg);
}

auto ShapePointsUndoAction::undo(Control* control) -> bool {
    apply(control, before);
    return true;
}

auto ShapePointsUndoAction::redo(Control* control) -> bool {
    apply(control, after);
    return true;
}

auto ShapePointsUndoAction::getText() -> std::string { return _("Edit shape"); }
