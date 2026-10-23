/*
 * Xournal++
 *
 * Undo action for editing the corners of a shape (see ShapePoints)
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <string>  // for string
#include <vector>  // for vector

#include "model/PageRef.h"  // for PageRef
#include "model/Point.h"    // for Point

#include "UndoAction.h"  // for UndoAction

class Control;
class Stroke;

class ShapePointsUndoAction: public UndoAction {
public:
    ShapePointsUndoAction(const PageRef& page, Stroke* stroke, std::vector<Point> before, std::vector<Point> after);

    bool undo(Control* control) override;
    bool redo(Control* control) override;
    std::string getText() override;

private:
    void apply(Control* control, const std::vector<Point>& points);

    Stroke* stroke;
    std::vector<Point> before;
    std::vector<Point> after;
};
