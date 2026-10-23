/*
 * Xournal++
 *
 * Part of the customizable toolbars: slider choosing the line width of the current tool (pen, highlighter, eraser)
 * freely. Marks show the tool's sizes (very fine ... very thick); the slider snaps to them when close.
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <string>  // for string

#include <gtk/gtk.h>  // for GtkWidget

#include "gui/IconNameHelper.h"     // for IconNameHelper
#include "util/raii/GObjectSPtr.h"  // for WidgetSPtr

#include "AbstractSliderItem.h"  // for AbstractSliderItem

class ActionDatabase;
class ToolHandler;

class ToolThicknessSlider: public AbstractSliderItem {
public:
    ToolThicknessSlider(std::string id, ToolHandler* toolHandler, IconNameHelper iconNameHelper, ActionDatabase& db);
    ~ToolThicknessSlider() override = default;

    xoj::util::WidgetSPtr createItem(bool horizontal) override;

    /// Widths offered when the slider is created (in points): 0.15 mm (the pen's thinnest size) to 5 mm. The range
    /// then follows the current tool, from its thinnest size to its thickest (at least MAX_THICKNESS)
    static constexpr double MIN_THICKNESS = 0.42;
    static constexpr double MAX_THICKNESS = 5.0 * 72.0 / 25.4;

protected:
    static constexpr bool DISPLAY_VALUE = true;
    /// Shows the width in millimetres
    static std::string formatSliderValue(double value);

    std::string getToolDisplayName() const override;
    GtkWidget* getNewToolIcon() const override;

    /// Logarithmic scale: thin widths are not squeezed at one end
    static double scaleFunction(double x);
    static double scaleInverseFunction(double x);

protected:
    std::string iconName;
    ToolHandler* toolHandler;
    ActionDatabase& db;

    template <class FinalSliderType>
    friend class SliderItemCreationHelper;
};
