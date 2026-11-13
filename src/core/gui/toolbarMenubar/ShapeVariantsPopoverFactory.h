/*
 * Xournal++
 *
 * Part of the customizable toolbars: drop-down menu of a shape listing its variants (e.g. ellipse and
 * center-of-mass symbol) as a column of icon-only buttons. Choosing one selects that drawing type.
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <string>  // for string
#include <vector>  // for vector

#include <gtk/gtk.h>  // for GtkWidget

#include "enums/Action.enum.h"  // for Action
#include "gui/PopoverFactory.h"

class ShapeVariantsPopoverFactory final: public PopoverFactory {
public:
    struct Variant {
        Action action;        ///< The (boolean) action selecting the drawing type, e.g. Action::TOOL_DRAW_ELLIPSE
        std::string icon;     ///< Full icon name (see IconNameHelper)
        std::string tooltip;  ///< Shown on hover, as the buttons have no text
    };

    explicit ShapeVariantsPopoverFactory(std::vector<Variant> variants);
    ~ShapeVariantsPopoverFactory() override = default;

    GtkWidget* createPopover() const override;

private:
    std::vector<Variant> variants;
};
