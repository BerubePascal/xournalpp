#include "ToolThicknessSlider.h"

#include <algorithm>  // for max
#include <cmath>    // for exp, log
#include <cstdio>   // for snprintf
#include <memory>   // for make_unique
#include <utility>  // for move

#include "control/Tool.h"                    // for Tool
#include "control/ToolHandler.h"             // for ToolHandler
#include "control/actions/ActionDatabase.h"  // for ActionDatabase
#include "util/GVariantTemplate.h"              // for getGVariantValue
#include "util/i18n.h"                       // for _
#include "util/raii/GVariantSPtr.h"          // for GVariantSPtr

#include "SliderItemCreationHelper.h"  // for SliderItemCreationHelper

namespace {
constexpr int FINE_STEP_COUNT = 200;
constexpr int COARSE_STEP_COUNT = 10;
constexpr double POINTS_PER_MM = 72.0 / 25.4;

constexpr auto SLIDER_RANGE = AbstractSliderItem::SliderRange{
        ToolThicknessSlider::MIN_THICKNESS, ToolThicknessSlider::MAX_THICKNESS, FINE_STEP_COUNT, COARSE_STEP_COUNT};
}  // namespace

ToolThicknessSlider::ToolThicknessSlider(std::string id, ToolHandler* toolHandler, IconNameHelper iconNameHelper,
                                         ActionDatabase& db):
        AbstractSliderItem{std::move(id), Category::TOOLS, SLIDER_RANGE, db.getAction(Action::TOOL_THICKNESS)},
        iconName(iconNameHelper.iconName("thickness-medium")),
        toolHandler(toolHandler),
        db(db) {}

auto ToolThicknessSlider::formatSliderValue(double value) -> std::string {
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%.2f mm", scaleInverseFunction(value) / POINTS_PER_MM);
    return buf;
}

auto ToolThicknessSlider::createItem(bool horizontal) -> xoj::util::WidgetSPtr {
    auto item = SliderItemCreationHelper<ToolThicknessSlider>::createItem(this, horizontal);
    // 1.5x longer than the default slider length, for finer control
    constexpr int LENGTH = 180;
    if (horizontal) {
        gtk_widget_set_size_request(item.get(), LENGTH, 16);
    } else {
        gtk_widget_set_size_request(item.get(), 16, LENGTH);
    }

    /// Range and marks (at the sizes) of the current tool, updated when the tool changes
    struct Marks {
        GtkScale* slider;
        ToolHandler* toolHandler;
        GAction* action;  ///< The TOOL_THICKNESS action, synced with the slider

        static void update(GObject*, GParamSpec*, Marks* self) {
            gtk_scale_clear_marks(self->slider);
            const Tool& tool = self->toolHandler->getActiveTool() ? *self->toolHandler->getActiveTool() :
                                                                     self->toolHandler->getTool(TOOL_PEN);
            if (!tool.hasCapability(TOOL_CAP_SIZE)) {
                return;
            }

            // The range follows the tool: from its thinnest size to its thickest, and at least up to MAX_THICKNESS
            const double min = scaleFunction(tool.getThickness(TOOL_SIZE_VERY_FINE));
            const double max = scaleFunction(std::max(MAX_THICKNESS, tool.getThickness(TOOL_SIZE_VERY_THICK)));
            GtkRange* range = GTK_RANGE(self->slider);
            gpointer action = self->action;
            // Changing the range may move the slider: this must not change the tool's width
            g_signal_handlers_block_by_func(range, (gpointer)SliderItemCreationHelper<ToolThicknessSlider>::valueChangedCb,
                                            action);
            gtk_range_set_range(range, min, max);
            gtk_range_set_increments(range, (max - min) / FINE_STEP_COUNT, (max - min) / COARSE_STEP_COUNT);
            xoj::util::GVariantSPtr state(g_action_get_state(G_ACTION(action)), xoj::util::adopt);
            gtk_range_set_value(range, scaleFunction(getGVariantValue<double>(state.get())));
            g_signal_handlers_unblock_by_func(
                    range, (gpointer)SliderItemCreationHelper<ToolThicknessSlider>::valueChangedCb, action);

            auto position = gtk_orientable_get_orientation(GTK_ORIENTABLE(self->slider)) == GTK_ORIENTATION_HORIZONTAL ?
                                    GTK_POS_BOTTOM :
                                    GTK_POS_RIGHT;
            for (int s = TOOL_SIZE_VERY_FINE; s <= TOOL_SIZE_VERY_THICK; s++) {
                gtk_scale_add_mark(self->slider, scaleFunction(tool.getThickness(static_cast<ToolSize>(s))), position,
                                   nullptr);
            }
        }
    };

    auto marks = std::make_unique<Marks>(Marks{GTK_SCALE(item.get()), toolHandler, G_ACTION(gAction.get())});
    Marks::update(nullptr, nullptr, marks.get());

    GObject* selectToolAction = G_OBJECT(db.getAction(Action::SELECT_TOOL).get());
    g_signal_connect(selectToolAction, "notify::state", G_CALLBACK(Marks::update), marks.get());

    // Disconnect the signal and destroy *marks if the widget is destroyed
    struct Cleanup {
        std::unique_ptr<Marks> marks;
        GObject* action;
    };
    g_object_weak_ref(
            G_OBJECT(item.get()),
            +[](gpointer d, GObject*) {
                auto* cleanup = static_cast<Cleanup*>(d);
                g_signal_handlers_disconnect_by_data(cleanup->action, cleanup->marks.get());
                delete cleanup;
            },
            new Cleanup{std::move(marks), selectToolAction});

    return item;
}

auto ToolThicknessSlider::getToolDisplayName() const -> std::string { return _("Line Width Slider"); }

auto ToolThicknessSlider::getNewToolIcon() const -> GtkWidget* {
    return gtk_image_new_from_icon_name(iconName.c_str(), GTK_ICON_SIZE_SMALL_TOOLBAR);
}

auto ToolThicknessSlider::scaleFunction(double x) -> double { return std::log(x); }

auto ToolThicknessSlider::scaleInverseFunction(double x) -> double { return std::exp(x); }
