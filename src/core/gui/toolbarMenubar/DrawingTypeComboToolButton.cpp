#include "DrawingTypeComboToolButton.h"

#include <algorithm>  // for find_if
#include <optional>   // for optional
#include <utility>    // for move

#include <gtk/gtk.h>

#include "control/actions/ActionDatabase.h"
#include "gui/IconNameHelper.h"
#include "util/glib_casts.h"
#include "util/gtk4_helper.h"  // for gtk_popover_new
#include "util/i18n.h"

#include "ArrowheadPopoverFactory.h"      // for ArrowheadPopoverFactory
#include "ShapeVariantsPopoverFactory.h"  // for ShapeVariantsPopoverFactory

/// Returns a floating ref
static GtkWidget* createPopoverEntry(const DrawingTypeComboToolButton::Entry& e) {
    GtkWidget* entry = gtk_toggle_button_new();
    gtk_widget_set_can_focus(entry, false);  // todo(gtk4) not necessary anymore
    auto actionName = std::string("win.") + g_action_get_name(G_ACTION(e.gAction.get()));
    gtk_actionable_set_action_name(GTK_ACTIONABLE(entry), actionName.data());
    GtkBox* box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6));
    gtk_button_set_child(GTK_BUTTON(entry), GTK_WIDGET(box));
    gtk_box_append(box, gtk_image_new_from_icon_name(e.icon.c_str(), GTK_ICON_SIZE_SMALL_TOOLBAR));
    gtk_box_append(box, gtk_label_new(e.name.c_str()));
    gtk_widget_set_tooltip_text(entry, e.name.c_str());
    return entry;
}

/// The action holding the arrowhead mode of the shape, if it can have arrowheads
static auto arrowheadsActionFor(DrawingTypeComboToolButton::Type t) -> std::optional<Action> {
    using Type = DrawingTypeComboToolButton::Type;
    switch (t) {
        case Type::LINE:
            return Action::TOOL_ARROWHEADS_LINE;
        case Type::SPLINE:
            return Action::TOOL_ARROWHEADS_SPLINE;
        case Type::COORDINATE_SYSTEM:
            return Action::TOOL_ARROWHEADS_COORDINATE_SYSTEM;
        case Type::ARC:
            return Action::TOOL_ARROWHEADS_ARC;
        default:
            return std::nullopt;
    }
}

/// The variants of the shape, shown in a submenu of its entry (empty if it has none)
static auto variantsFor(DrawingTypeComboToolButton::Type t) -> std::vector<DrawingTypeComboToolButton::Type> {
    using Type = DrawingTypeComboToolButton::Type;
    switch (t) {
        case Type::ELLIPSE:
            return {Type::ELLIPSE, Type::MASS_CENTER};
        default:
            return {};
    }
}

/// Shapes only listed in the submenu of another shape (see variantsFor())
static bool isVariantOnly(DrawingTypeComboToolButton::Type t) {
    return t == DrawingTypeComboToolButton::Type::MASS_CENTER;
}

static auto makeEntries(IconNameHelper& icons, const ActionDatabase& db)
        -> std::shared_ptr<EnumIndexedArray<DrawingTypeComboToolButton::Entry, DrawingTypeComboToolButton::Type>> {
    using Type = DrawingTypeComboToolButton::Type;
    using Entry = DrawingTypeComboToolButton::Entry;
    auto res = std::make_shared<EnumIndexedArray<Entry, Type>>();
    auto& entries = *res;
    entries[Type::RECTANGLE] = Entry(_("Draw Rectangle"), icons.iconName("draw-rect"), db, Action::TOOL_DRAW_RECTANGLE);
    entries[Type::ELLIPSE] = Entry(_("Draw Ellipse"), icons.iconName("draw-ellipse"), db, Action::TOOL_DRAW_ELLIPSE);
    entries[Type::ARROW] = Entry(_("Draw Arrow"), icons.iconName("draw-arrow"), db, Action::TOOL_DRAW_ARROW);
    entries[Type::DOUBLE_ARROW] =
            Entry(_("Draw Double Arrow"), icons.iconName("draw-double-arrow"), db, Action::TOOL_DRAW_DOUBLE_ARROW);
    entries[Type::LINE] = Entry(_("Draw Line"), icons.iconName("draw-line"), db, Action::TOOL_DRAW_LINE);
    entries[Type::COORDINATE_SYSTEM] = Entry(_("Draw coordinate system"), icons.iconName("draw-coordinate-system"), db,
                                             Action::TOOL_DRAW_COORDINATE_SYSTEM);
    entries[Type::SPLINE] = Entry(_("Draw Spline"), icons.iconName("draw-spline"), db, Action::TOOL_DRAW_SPLINE);
    entries[Type::ARC] = Entry(_("Draw Arc"), icons.iconName("draw-arc"), db, Action::TOOL_DRAW_ARC);
    entries[Type::MASS_CENTER] =
            Entry(_("Draw Center of Mass"), icons.iconName("draw-mass-center"), db, Action::TOOL_DRAW_MASS_CENTER);
    entries[Type::SHAPE_RECOGNIZER] =
            Entry(_("Stroke recognizer"), icons.iconName("shape-recognizer"), db, Action::TOOL_DRAW_SHAPE_RECOGNIZER);
    for (size_t n = 0; n < xoj::to_underlying(Type::ENUMERATOR_COUNT); n++) {
        if (auto a = arrowheadsActionFor(static_cast<Type>(n))) {
            entries[static_cast<Type>(n)].arrowheadsAction = db.getAction(*a);
        }
    }
    return res;
}

DrawingTypeComboToolButton::Entry::Entry(std::string name, std::string icon, const ActionDatabase& db, Action a):
        name(std::move(name)),
        icon(std::move(icon)),
        action(a),
        gAction(db.getAction(a)),
        fullActionName(std::string("win.") + g_action_get_name(G_ACTION(gAction.get()))) {}

DrawingTypeComboToolButton::DrawingTypeComboToolButton(std::string id, IconNameHelper& icons, const ActionDatabase& db):
        AbstractToolItem(std::move(id), Category::TOOLS),
        entries(makeEntries(icons, db)),
        iconName(icons.iconName("combo-drawing-type")),
        description(_("Drawing Type Combo")) {}

DrawingTypeComboToolButton::~DrawingTypeComboToolButton() = default;

/// Data structure for callbacks
struct Data {
    GtkButton* button;
    GtkPopover* popover;
    std::shared_ptr<const EnumIndexedArray<DrawingTypeComboToolButton::Entry, DrawingTypeComboToolButton::Type>>
            entries;

    /// Icon of the entry, with its current arrowheads
    static std::string iconOf(const DrawingTypeComboToolButton::Entry& e) {
        return arrowheadIconName(e.icon, e.arrowheadsAction.get());
    }

    /// The arrowheads of a shape changed: update the prominent icon if it shows that shape
    static void arrowheadsChangedCallback(GObject*, GParamSpec*, Data* self) {
        for (const auto& e: *self->entries) {
            xoj::util::GVariantSPtr state(g_action_get_state(G_ACTION(e.gAction.get())), xoj::util::adopt);
            if (getGVariantValue<bool>(state.get())) {
                gtk_button_set_icon_name(self->button, iconOf(e).c_str());
                return;
            }
        }
    }

    template <DrawingTypeComboToolButton::Type s>
    static void setProminentIconCallback(GObject* action, GParamSpec*, Data* self) {
        xoj::util::GVariantSPtr state(g_action_get_state(G_ACTION(action)), xoj::util::adopt);
        if (getGVariantValue<bool>(state.get())) {
            auto& e = (*self->entries)[s];
            gtk_button_set_icon_name(self->button, iconOf(e).c_str());
            gtk_widget_set_tooltip_text(GTK_WIDGET(self->button), e.name.c_str());
            gtk_actionable_set_action_name(GTK_ACTIONABLE(self->button), e.fullActionName.c_str());
        }
        gtk_popover_popdown(self->popover);
    }
};

auto DrawingTypeComboToolButton::createItem(bool horizontal) -> xoj::util::WidgetSPtr {
    auto data = std::make_unique<Data>();
    data->entries = this->entries;

    {  // Create popover
        data->popover = GTK_POPOVER(gtk_popover_new());
        GtkBox* box = GTK_BOX(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0));
        gtk_popover_set_child(data->popover, GTK_WIDGET(box));
        gtk_widget_add_css_class(GTK_WIDGET(data->popover), "toolbar");

        for (size_t n = 0; n < xoj::to_underlying(Type::ENUMERATOR_COUNT); n++) {
            const Entry& t = (*this->entries)[static_cast<Type>(n)];
            if (isVariantOnly(static_cast<Type>(n))) {
                continue;
            }
            std::optional<Action> arrowheadsAction = arrowheadsActionFor(static_cast<Type>(n));
            std::vector<Type> variants = variantsFor(static_cast<Type>(n));
            if (!arrowheadsAction && variants.empty()) {
                gtk_box_append(box, createPopoverEntry(t));
                continue;
            }
            // A ▸ on the right of the entry opens the shape's arrowhead choices (shapes with open ends)
            // or its variants (e.g. ellipse / center of mass)
            GtkBox* row = GTK_BOX(gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0));
            GtkWidget* entry = createPopoverEntry(t);
            gtk_widget_set_hexpand(entry, true);
            gtk_box_append(row, entry);

            GtkMenuButton* subButton = GTK_MENU_BUTTON(gtk_menu_button_new());
            gtk_widget_set_can_focus(GTK_WIDGET(subButton), false);  // todo(gtk4) not necessary anymore
            gtk_menu_button_set_direction(subButton, GTK_ARROW_RIGHT);
            if (arrowheadsAction) {
                gtk_widget_set_tooltip_text(GTK_WIDGET(subButton), _("Arrowheads"));
                gtk_menu_button_set_popover(
                        subButton, ArrowheadPopoverFactory(t.gAction, *arrowheadsAction, t.icon).createPopover());
            } else {
                std::vector<ShapeVariantsPopoverFactory::Variant> vs;
                for (Type v: variants) {
                    const Entry& ve = (*this->entries)[v];
                    vs.push_back({ve.action, ve.icon, ve.name});
                }
                gtk_widget_set_tooltip_text(GTK_WIDGET(subButton), _("Variants"));
                gtk_menu_button_set_popover(subButton, ShapeVariantsPopoverFactory(std::move(vs)).createPopover());
            }
            gtk_box_append(row, GTK_WIDGET(subButton));

            gtk_box_append(box, GTK_WIDGET(row));
        }

        gtk_widget_show_all(GTK_WIDGET(box));
    }

    {  // Create prominent button
        GtkWidget* btn = gtk_toggle_button_new();
        data->button = GTK_BUTTON(btn);

        gtk_widget_set_can_focus(btn, false);  // todo(gtk4) not necessary anymore

        auto it = std::find_if(entries->begin(), entries->end(), [](auto& e) {
            xoj::util::GVariantSPtr state(g_action_get_state(G_ACTION(e.gAction.get())), xoj::util::adopt);
            return getGVariantValue<bool>(state.get());
        });
        const Entry& e = it == entries->end() ? (*entries)[Type::RECTANGLE] : *it;  // Select an entry by default
        gtk_actionable_set_action_name(GTK_ACTIONABLE(btn), e.fullActionName.data());
        gtk_button_set_icon_name(GTK_BUTTON(btn), Data::iconOf(e).c_str());
        gtk_widget_set_tooltip_text(btn, e.name.c_str());
    }

    // Create item
    GtkMenuButton* menubutton = GTK_MENU_BUTTON(gtk_menu_button_new());
    gtk_widget_set_can_focus(GTK_WIDGET(menubutton), false);  // todo(gtk4) not necessary anymore
    gtk_menu_button_set_popover(menubutton, GTK_WIDGET(data->popover));
    gtk_menu_button_set_direction(menubutton,
                                  horizontal ? GTK_ARROW_DOWN : GTK_ARROW_RIGHT);  // TODO: fix directions

    GtkBox* box = GTK_BOX(gtk_box_new(horizontal ? GTK_ORIENTATION_HORIZONTAL : GTK_ORIENTATION_VERTICAL, 0));
    gtk_box_append(box, GTK_WIDGET(data->button));
    gtk_box_append(box, GTK_WIDGET(menubutton));

    auto item = xoj::util::WidgetSPtr(GTK_WIDGET(box), xoj::util::adopt);

    /// change the prominent icon when selecting an entry (by any means)
    g_signal_connect((*entries)[Type::RECTANGLE].gAction.get(), "notify::state",
                     xoj::util::wrap_for_g_callback_v<Data::setProminentIconCallback<Type::RECTANGLE>>, data.get());
    g_signal_connect((*entries)[Type::ELLIPSE].gAction.get(), "notify::state",
                     xoj::util::wrap_for_g_callback_v<Data::setProminentIconCallback<Type::ELLIPSE>>, data.get());
    g_signal_connect((*entries)[Type::ARROW].gAction.get(), "notify::state",
                     xoj::util::wrap_for_g_callback_v<Data::setProminentIconCallback<Type::ARROW>>, data.get());
    g_signal_connect((*entries)[Type::DOUBLE_ARROW].gAction.get(), "notify::state",
                     xoj::util::wrap_for_g_callback_v<Data::setProminentIconCallback<Type::DOUBLE_ARROW>>, data.get());
    g_signal_connect((*entries)[Type::LINE].gAction.get(), "notify::state",
                     xoj::util::wrap_for_g_callback_v<Data::setProminentIconCallback<Type::LINE>>, data.get());
    g_signal_connect((*entries)[Type::COORDINATE_SYSTEM].gAction.get(), "notify::state",
                     xoj::util::wrap_for_g_callback_v<Data::setProminentIconCallback<Type::COORDINATE_SYSTEM>>,
                     data.get());
    g_signal_connect((*entries)[Type::SPLINE].gAction.get(), "notify::state",
                     xoj::util::wrap_for_g_callback_v<Data::setProminentIconCallback<Type::SPLINE>>, data.get());
    g_signal_connect((*entries)[Type::ARC].gAction.get(), "notify::state",
                     xoj::util::wrap_for_g_callback_v<Data::setProminentIconCallback<Type::ARC>>, data.get());
    g_signal_connect((*entries)[Type::MASS_CENTER].gAction.get(), "notify::state",
                     xoj::util::wrap_for_g_callback_v<Data::setProminentIconCallback<Type::MASS_CENTER>>,
                     data.get());
    g_signal_connect((*entries)[Type::SHAPE_RECOGNIZER].gAction.get(), "notify::state",
                     xoj::util::wrap_for_g_callback_v<Data::setProminentIconCallback<Type::SHAPE_RECOGNIZER>>,
                     data.get());

    for (const auto& e: *entries) {
        if (e.arrowheadsAction) {
            g_signal_connect(e.arrowheadsAction.get(), "notify::state",
                             xoj::util::wrap_for_g_callback_v<Data::arrowheadsChangedCallback>, data.get());
        }
    }

    // Disconnect the signal and destroy *data if the widget is destroyed
    g_object_weak_ref(
            G_OBJECT(item.get()),
            +[](gpointer d, GObject*) {
                Data* data = static_cast<Data*>(d);
                g_signal_handlers_disconnect_by_data((*data->entries)[Type::RECTANGLE].gAction.get(), d);
                g_signal_handlers_disconnect_by_data((*data->entries)[Type::ELLIPSE].gAction.get(), d);
                g_signal_handlers_disconnect_by_data((*data->entries)[Type::ARROW].gAction.get(), d);
                g_signal_handlers_disconnect_by_data((*data->entries)[Type::DOUBLE_ARROW].gAction.get(), d);
                g_signal_handlers_disconnect_by_data((*data->entries)[Type::LINE].gAction.get(), d);
                g_signal_handlers_disconnect_by_data((*data->entries)[Type::COORDINATE_SYSTEM].gAction.get(), d);
                g_signal_handlers_disconnect_by_data((*data->entries)[Type::SPLINE].gAction.get(), d);
                g_signal_handlers_disconnect_by_data((*data->entries)[Type::ARC].gAction.get(), d);
                g_signal_handlers_disconnect_by_data((*data->entries)[Type::MASS_CENTER].gAction.get(), d);
                g_signal_handlers_disconnect_by_data((*data->entries)[Type::SHAPE_RECOGNIZER].gAction.get(), d);
                for (const auto& e: *data->entries) {
                    if (e.arrowheadsAction) {
                        g_signal_handlers_disconnect_by_data(e.arrowheadsAction.get(), d);
                    }
                }
                delete data;
            },
            data.release());

    return item;
}

auto DrawingTypeComboToolButton::getToolDisplayName() const -> std::string { return this->description; }

auto DrawingTypeComboToolButton::getNewToolIcon() const -> GtkWidget* {
    return gtk_image_new_from_icon_name(iconName.c_str(), GTK_ICON_SIZE_SMALL_TOOLBAR);
}
