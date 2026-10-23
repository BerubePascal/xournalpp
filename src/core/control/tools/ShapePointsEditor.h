/*
 * Xournal++
 *
 * Handles of a selected simple shape (see ShapePoints): its corners, or the start and end of an arc. Draws them,
 * finds the one under the cursor and drags it. Used by EditSelection when the selection contains a single simple
 * shape.
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <cstddef>   // for size_t
#include <memory>    // for unique_ptr
#include <optional>  // for optional
#include <vector>    // for vector

#include <cairo.h>    // for cairo_t
#include <gdk/gdk.h>  // for GdkRGBA

#include "model/ElementInsertionPosition.h"  // for InsertionOrder
#include "model/PageRef.h"                   // for PageRef
#include "model/Point.h"                     // for Point
#include "undo/UndoAction.h"                 // for UndoActionPtr

#include "ShapePoints.h"  // for ShapePoints::Shape

class SnapToGridInputHandler;
class Stroke;

class ShapePointsEditor {
public:
    /// nullptr unless elements is a single simple shape
    static std::unique_ptr<ShapePointsEditor> create(const InsertionOrder& elements);

    ShapePointsEditor(Stroke* stroke, ShapePoints::Shape shape);

    /// Index of the handle at (x, y) (document coordinates), if any
    std::optional<size_t> handleAt(double x, double y, double tolerance) const;

    void paintHandles(cairo_t* cr, double zoom, const GdkRGBA& color, double size) const;

    void startDrag(size_t handle);
    /**
     * Moves the dragged handle towards p (document coordinates). Corners snap to the grid; the ends of an arc slide
     * along its circle and its angle snaps like rotations. alt toggles snapping.
     */
    void dragTo(const Point& p, bool alt, const SnapToGridInputHandler& snapping);
    /// Ends the drag. Returns the undo action, or nullptr if nothing changed
    UndoActionPtr endDrag(const PageRef& page);

    Stroke* getStroke() const;

private:
    Stroke* stroke;
    ShapePoints::Shape shape;
    std::optional<size_t> dragged;
    std::vector<Point> pointsBeforeDrag;

    /// Arcs: angles followed continuously while dragging (before snapping), see dragTo()
    double dragLastAngle = 0;
    double dragStartAngle = 0;
    double dragSweep = 0;
};
