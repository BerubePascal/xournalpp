/*
 * Xournal++
 *
 * Part of the customizable toolbars: drop-down menu of a shape (line, spline, coordinate system) to choose its
 * arrowheads. Choosing an entry also selects the shape.
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <gtk/gtk.h>  // for GtkWidget

#include "control/actions/ActionRef.h"  // for ActionRef
#include "enums/Action.enum.h"          // for Action
#include "gui/PopoverFactory.h"

class ArrowheadPopoverFactory final: public PopoverFactory {
public:
    /**
     * @param shapeAction The (boolean) action selecting the shape, e.g. the one of Action::TOOL_DRAW_LINE
     * @param arrowheadsAction The action holding the shape's arrowhead mode, e.g. Action::TOOL_ARROWHEADS_LINE
     */
    ArrowheadPopoverFactory(ActionRef shapeAction, Action arrowheadsAction);
    ~ArrowheadPopoverFactory() override = default;

    GtkWidget* createPopover() const override;

private:
    ActionRef shapeAction;
    Action arrowheadsAction;
};
