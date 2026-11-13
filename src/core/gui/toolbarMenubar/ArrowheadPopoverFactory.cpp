#include "ArrowheadPopoverFactory.h"

#include <utility>  // for move

#include <glib.h>

#include "util/GtkUtil.h"
#include "util/gtk4_helper.h"
#include "util/i18n.h"                 // for _
#include "util/raii/GVariantSPtr.h"    // for GVariantSPtr

std::string arrowheadIconName(const std::string& baseIcon, const char* mode) {
    if (mode == nullptr || g_strcmp0(mode, "none") == 0) {
        return baseIcon;
    }
    std::string name = baseIcon + "-" + mode;
    return gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), name.c_str()) ? name : baseIcon;
}

std::string arrowheadIconName(const std::string& baseIcon, GSimpleAction* arrowheadsAction) {
    if (!arrowheadsAction) {
        return baseIcon;
    }
    xoj::util::GVariantSPtr state(g_action_get_state(G_ACTION(arrowheadsAction)), xoj::util::adopt);
    if (!state || !g_variant_is_of_type(state.get(), G_VARIANT_TYPE_STRING)) {
        return baseIcon;
    }
    return arrowheadIconName(baseIcon, g_variant_get_string(state.get(), nullptr));
}

ArrowheadPopoverFactory::ArrowheadPopoverFactory(ActionRef shapeAction, Action arrowheadsAction,
                                                 std::string baseIcon):
        shapeAction(std::move(shapeAction)), arrowheadsAction(arrowheadsAction), baseIcon(std::move(baseIcon)) {}

GtkWidget* ArrowheadPopoverFactory::createPopover() const {
    GtkWidget* popover = gtk_popover_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_popover_set_child(GTK_POPOVER(popover), box);
    gtk_widget_add_css_class(popover, "toolbar");

    struct Entry {
        const char* label;
        const char* target;
    };
    const Entry entries[] = {{_("Normal"), "none"},
                             {_("Arrowhead at start"), "start"},
                             {_("Arrowhead at end"), "end"},
                             {_("Arrowheads at both ends"), "both"}};

    GtkRadioButton* group = nullptr;
    for (const auto& e: entries) {
        // Icon-only buttons (radio buttons drawn as toggle buttons): the current mode looks pressed
        GtkWidget* btn = gtk_radio_button_new_from_widget(group);
        group = GTK_RADIO_BUTTON(btn);
        gtk_toggle_button_set_mode(GTK_TOGGLE_BUTTON(btn), false);
        gtk_widget_set_can_focus(btn, false);  // todo(gtk4) not necessary anymore
        gtk_button_set_child(GTK_BUTTON(btn),
                             gtk_image_new_from_icon_name(arrowheadIconName(baseIcon, e.target).c_str(),
                                                          GTK_ICON_SIZE_LARGE_TOOLBAR));
        gtk_widget_set_tooltip_text(btn, e.label);
        xoj::util::gtk::setRadioButtonActionName(GTK_RADIO_BUTTON(btn), "win", Action_toString(arrowheadsAction));
        gtk_actionable_set_action_target_value(GTK_ACTIONABLE(btn), g_variant_new_string(e.target));
        gtk_box_append(GTK_BOX(box), btn);

        // Choosing an entry also selects the shape. The action is owned by the ActionDatabase, which outlives the UI.
        g_object_set_data(G_OBJECT(btn), "xoj-shape-action", shapeAction.get());
        g_signal_connect_object(btn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer popover) {
                                    // Also emitted when the arrowhead mode changes elsewhere (e.g. keyboard
                                    // shortcut): only react to the user's click in the open popover
                                    if (!gtk_widget_get_mapped(GTK_WIDGET(btn)) ||
                                        !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn))) {
                                        return;
                                    }
                                    auto* a = G_ACTION(g_object_get_data(G_OBJECT(btn), "xoj-shape-action"));
                                    g_action_change_state(a, g_variant_new_boolean(true));

                                    gtk_popover_popdown(GTK_POPOVER(popover));
                                    // If this popover was opened from another popover (e.g. the Drawing Type menu),
                                    // close that one too
                                    if (GtkWidget* from = gtk_popover_get_relative_to(GTK_POPOVER(popover))) {
                                        if (GtkWidget* outer = gtk_widget_get_ancestor(from, GTK_TYPE_POPOVER)) {
                                            gtk_popover_popdown(GTK_POPOVER(outer));
                                        }
                                    }
                                }),
                                popover, GConnectFlags(0));
    }

    gtk_widget_show_all(box);
    return popover;
}
