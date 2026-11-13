/*
 * Xournal++
 *
 * Part of the customizable toolbars: drop-down menu of a shape (line, spline, coordinate system, arc) to choose its
 * arrowheads, as a column of icon-only buttons. Choosing an entry also selects the shape.
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <string>  // for string

#include <gtk/gtk.h>  // for GtkWidget

#include "control/actions/ActionRef.h"  // for ActionRef
#include "enums/Action.enum.h"          // for Action
#include "gui/PopoverFactory.h"

/**
 * @brief Icon of a shape with arrowheads, e.g. "xopp-draw-line-end" for ("xopp-draw-line", "end").
 *      Falls back to the plain icon for "none" or if the icon theme has no such icon.
 * @param mode The arrowhead mode, as in the state of the arrowhead actions ("none", "start", "end" or "both")
 */
std::string arrowheadIconName(const std::string& baseIcon, const char* mode);

/// Same, reading the mode from the state of an arrowhead action
std::string arrowheadIconName(const std::string& baseIcon, GSimpleAction* arrowheadsAction);

class ArrowheadPopoverFactory final: public PopoverFactory {
public:
    /**
     * @param shapeAction The (boolean) action selecting the shape, e.g. the one of Action::TOOL_DRAW_LINE
     * @param arrowheadsAction The action holding the shape's arrowhead mode, e.g. Action::TOOL_ARROWHEADS_LINE
     * @param baseIcon The shape's plain icon (full name), e.g. "xopp-draw-line"
     */
    ArrowheadPopoverFactory(ActionRef shapeAction, Action arrowheadsAction, std::string baseIcon);
    ~ArrowheadPopoverFactory() override = default;

    GtkWidget* createPopover() const override;

private:
    ActionRef shapeAction;
    Action arrowheadsAction;
    std::string baseIcon;
};
