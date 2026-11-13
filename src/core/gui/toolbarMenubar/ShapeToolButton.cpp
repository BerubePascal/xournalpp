#include "ShapeToolButton.h"

#include <utility>  // for move

#include "control/actions/ActionDatabase.h"  // for ActionDatabase
#include "util/GVariantTemplate.h"           // for getGVariantValue
#include "util/gtk4_helper.h"                // for gtk_button_get_child
#include "util/raii/GVariantSPtr.h"          // for GVariantSPtr

#include "ArrowheadPopoverFactory.h"  // for arrowheadIconName

namespace {
constexpr const char* DATA_KEY = "xoj-shape-button-data";

/// Per created button
struct ButtonData {
    std::shared_ptr<const ShapeToolButton::Config> config;
    GtkImage* image;
    size_t currentVariant = 0;  ///< Variant chosen last (shown when none is selected)
};

bool isActive(GSimpleAction* a) {
    xoj::util::GVariantSPtr state(g_action_get_state(G_ACTION(a)), xoj::util::adopt);
    return state && g_variant_is_of_type(state.get(), G_VARIANT_TYPE_BOOLEAN) && getGVariantValue<bool>(state.get());
}

/// Update the button's icon (and, for variants, its action) from the current state of the actions
void refresh(GtkWidget* btn) {
    auto* d = static_cast<ButtonData*>(g_object_get_data(G_OBJECT(btn), DATA_KEY));
    if (!d) {
        return;
    }
    const auto& cfg = *d->config;
    std::string icon = cfg.baseIcon;
    if (!cfg.variants.empty()) {
        for (size_t i = 0; i < cfg.variants.size(); i++) {
            if (isActive(cfg.variants[i].gAction.get())) {
                if (i != d->currentVariant) {
                    d->currentVariant = i;
                    gtk_actionable_set_action_name(GTK_ACTIONABLE(btn), cfg.variants[i].fullActionName.c_str());
                }
                break;
            }
        }
        icon = cfg.variants[d->currentVariant].icon;
    }
    if (cfg.arrowheads) {
        icon = arrowheadIconName(icon, cfg.arrowheads.get());
    }
    gtk_image_set_from_icon_name(d->image, icon.c_str(), GTK_ICON_SIZE_LARGE_TOOLBAR);
}
}  // namespace

ShapeToolButton::ShapeToolButton(std::string id, Category cat, Action action, std::string iconName,
                                 std::string description, const ActionDatabase& db,
                                 const std::vector<std::pair<Action, std::string>>& variants,
                                 std::optional<Action> arrowheadsAction):
        ToolButton(std::move(id), cat, action, iconName, std::move(description), true) {
    auto cfg = std::make_shared<Config>();
    cfg->baseIcon = std::move(iconName);
    for (const auto& [a, icon]: variants) {
        cfg->variants.push_back({db.getAction(a), std::string("win.") + Action_toString(a), icon});
    }
    if (arrowheadsAction) {
        cfg->arrowheads = db.getAction(*arrowheadsAction);
    }
    this->config = std::move(cfg);
}

void ShapeToolButton::onButtonCreated(GtkWidget* btn) {
    auto* image = GTK_IMAGE(gtk_button_get_child(GTK_BUTTON(btn)));
    g_object_set_data_full(G_OBJECT(btn), DATA_KEY, new ButtonData{config, image},
                           +[](gpointer d) { delete static_cast<ButtonData*>(d); });

    // The handlers are disconnected when the button is destroyed
    auto watch = [btn](GSimpleAction* a) {
        g_signal_connect_object(a, "notify::state",
                                G_CALLBACK(+[](GObject*, GParamSpec*, gpointer btn) { refresh(GTK_WIDGET(btn)); }),
                                btn, GConnectFlags(0));
    };
    for (const auto& v: config->variants) {
        watch(v.gAction.get());
    }
    if (config->arrowheads) {
        watch(config->arrowheads.get());
    }
    refresh(btn);
}
