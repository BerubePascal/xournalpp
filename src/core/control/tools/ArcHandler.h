/*
 * Xournal++
 *
 * Handles input to draw a circular arc in three steps:
 *  1. click: the center
 *  2. click: the radius and where the arc starts
 *  3. press, drag, release: where the arc ends. Between the 2nd and 3rd click the arc already follows the pointer.
 * The arc accumulates the angle the pointer sweeps through, so it goes the way the pointer went (it can exceed 180°)
 * instead of jumping to the other side of the circle.
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

class ArcHandler: public BaseShapeHandler {
public:
    ArcHandler(Control* control, const PageRef& page);
    ~ArcHandler() override;

    void onButtonPressEvent(const PositionInputData& pos, double zoom) override;
    void onButtonReleaseEvent(const PositionInputData& pos, double zoom) override;
    bool onKeyReleaseEvent(const KeyEvent& event) override;

    /// Abandon the arc being drawn (e.g. when changing tools)
    void cancel();

private:
    auto createShape(bool isAltDown, bool isShiftDown, bool isControlDown)
            -> std::pair<std::vector<Point>, Range> override;
    DrawingType getArrowheadShape() const override;

    enum class Step {
        CENTER_SET,  ///< The pointer chooses the radius and start of the arc
        START_SET,   ///< The pointer chooses the end of the arc (button released)
        END_DRAG     ///< The pointer chooses the end of the arc (button pressed): placed on release
    };
    Step step = Step::CENTER_SET;

    Point center;
    double radius = 0;
    double startAngle = 0;
    /// Signed angle swept by the pointer since the start of the arc (radians)
    double sweep = 0;
    /// Angle of the pointer (around the center) at the last update, to accumulate the sweep
    double lastAngle = 0;
};
