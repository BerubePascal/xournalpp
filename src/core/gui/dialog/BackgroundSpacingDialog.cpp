#include "BackgroundSpacingDialog.h"

#include <gtk/gtk.h>

#include "control/Control.h"                         // for Control
#include "control/PageBackgroundChangeController.h"  // for PageBackgroundChangeController
#include "model/BackgroundSpacing.h"                 // for BackgroundSpacing
#include "model/Document.h"                          // for Document
#include "model/XojPage.h"                           // for XojPage
#include "util/i18n.h"                               // for _

namespace {
constexpr double POINTS_PER_MM = 72.0 / 25.4;
constexpr double MIN_SPACING_MM = 1.0;
constexpr double MAX_SPACING_MM = 50.0;
constexpr double DEFAULT_SPACING_MM = 5.0;

struct DialogData {
    Control* control;
    GtkSpinButton* spacing;
    GtkToggleButton* allPages;
};
}  // namespace

void BackgroundSpacingDialog::show(Control* control) {
    // Start from the spacing of the current page
    double spacingMm = DEFAULT_SPACING_MM;
    bool currentPageHasSpacing = false;
    if (auto page = control->getCurrentPage()) {
        if (auto spacing = BackgroundSpacing::get(page->getBackgroundType())) {
            spacingMm = *spacing / POINTS_PER_MM;
            currentPageHasSpacing = true;
        }
    }

    GtkWidget* dialog = gtk_dialog_new_with_buttons(
            _("Background Spacing"), control->getGtkWindow(),
            static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), _("_Cancel"),
            GTK_RESPONSE_CANCEL, _("_Apply"), GTK_RESPONSE_ACCEPT, nullptr);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), GTK_RESPONSE_ACCEPT);

    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_container_set_border_width(GTK_CONTAINER(grid), 12);

    GtkWidget* label = gtk_label_new(_("Spacing of the lines / grid (mm):"));
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    GtkWidget* spin = gtk_spin_button_new_with_range(MIN_SPACING_MM, MAX_SPACING_MM, 0.5);
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), 2);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), spacingMm);
    gtk_entry_set_activates_default(GTK_ENTRY(spin), true);
    gtk_grid_attach(GTK_GRID(grid), spin, 1, 0, 1, 1);

    GtkWidget* thisPage = gtk_radio_button_new_with_label(nullptr, _("This page"));
    GtkWidget* allPages =
            gtk_radio_button_new_with_label_from_widget(GTK_RADIO_BUTTON(thisPage), _("All pages"));
    gtk_grid_attach(GTK_GRID(grid), thisPage, 0, 1, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), allPages, 0, 2, 2, 1);
    if (!currentPageHasSpacing) {
        // The current page has no lines or grid (e.g. plain): only "all pages" makes sense
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(allPages), true);
        gtk_widget_set_sensitive(thisPage, false);
    }

    GtkWidget* note = gtk_label_new(_("Applies to ruled, lined, graph, dotted and isometric backgrounds."));
    gtk_widget_set_halign(note, GTK_ALIGN_START);
    gtk_widget_set_opacity(note, 0.7);
    gtk_grid_attach(GTK_GRID(grid), note, 0, 3, 2, 1);

    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), grid);

    auto* data = new DialogData{control, GTK_SPIN_BUTTON(spin), GTK_TOGGLE_BUTTON(allPages)};
    g_signal_connect(dialog, "response", G_CALLBACK(+[](GtkDialog* dialog, int response, DialogData* data) {
                         if (response == GTK_RESPONSE_ACCEPT) {
                             gtk_spin_button_update(data->spacing);
                             double spacing = gtk_spin_button_get_value(data->spacing) * POINTS_PER_MM;
                             bool all = gtk_toggle_button_get_active(data->allPages);
                             data->control->getPageBackgroundChangeController()->changeBackgroundSpacing(spacing, all);
                         }
                         gtk_widget_destroy(GTK_WIDGET(dialog));
                     }),
                     data);
    g_object_weak_ref(
            G_OBJECT(dialog), +[](gpointer d, GObject*) { delete static_cast<DialogData*>(d); }, data);

    gtk_widget_show_all(dialog);
}
