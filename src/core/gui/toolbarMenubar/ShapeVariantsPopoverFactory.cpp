#include "ShapeVariantsPopoverFactory.h"

#include <utility>  // for move

#include "util/gtk4_helper.h"

ShapeVariantsPopoverFactory::ShapeVariantsPopoverFactory(std::vector<Variant> variants):
        variants(std::move(variants)) {}

GtkWidget* ShapeVariantsPopoverFactory::createPopover() const {
    GtkWidget* popover = gtk_popover_new();
    GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_popover_set_child(GTK_POPOVER(popover), box);
    gtk_widget_add_css_class(popover, "toolbar");

    for (const auto& v: variants) {
        GtkWidget* btn = gtk_toggle_button_new();
        gtk_widget_set_can_focus(btn, false);  // todo(gtk4) not necessary anymore
        gtk_actionable_set_action_name(GTK_ACTIONABLE(btn), (std::string("win.") + Action_toString(v.action)).c_str());
        gtk_button_set_child(GTK_BUTTON(btn), gtk_image_new_from_icon_name(v.icon.c_str(), GTK_ICON_SIZE_LARGE_TOOLBAR));
        gtk_widget_set_tooltip_text(btn, v.tooltip.c_str());
        gtk_box_append(GTK_BOX(box), btn);

        g_signal_connect_object(btn, "clicked", G_CALLBACK(+[](GtkButton* btn, gpointer popover) {
                                    // Also emitted when the drawing type changes elsewhere:
                                    // only react to a click in the open popover
                                    if (!gtk_widget_get_mapped(GTK_WIDGET(btn))) {
                                        return;
                                    }
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
