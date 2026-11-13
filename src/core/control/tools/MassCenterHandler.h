/*
 * Xournal++
 *
 * Handles input to draw a center-of-mass symbol: a circle split in four quadrants,
 * the top-left and bottom-right ones filled.
 * Press at the center, drag out to the radius, release.
 *
 * The symbol is added as two strokes (in one undo step), both in the current color with a solid line:
 *  - the two filled quadrants (fully opaque), whose edges also draw the cross;
 *  - the whole circle, not filled (the two other quadrants stay transparent).
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <utility>  // for pair
#include <vector>   // for vector

#include "model/PageRef.h"  // for PageRef
#include "model/Point.h"    // for Point
#include "util/Range.h"     // for Range

#include "BaseShapeHandler.h"  // for BaseShapeHandler

class Control;

class MassCenterHandler: public BaseShapeHandler {
public:
    MassCenterHandler(Control* control, const PageRef& page);
    ~MassCenterHandler() override;

    void onButtonPressEvent(const PositionInputData& pos, double zoom) override;
    void onButtonReleaseEvent(const PositionInputData& pos, double zoom) override;

    void addPreviewPath(cairo_t* cr) const override;

private:
    auto createShape(bool isAltDown, bool isShiftDown, bool isControlDown)
            -> std::pair<std::vector<Point>, Range> override;

    /// Points of the circle arc from angle a0 to a1 (radians, y pointing down), both ends included
    std::vector<Point> arc(double a0, double a1) const;

    Point center;
    double radius = 0;
};
