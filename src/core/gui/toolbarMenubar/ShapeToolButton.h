/*
 * Xournal++
 *
 * Part of the customizable toolbars: button of a shape whose icon follows the shape's current choice:
 *  - shapes with variants (e.g. ellipse / center of mass): shows the variant chosen last, and clicking selects it;
 *  - shapes with arrowheads (line, arc, ...): shows the current arrowheads.
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <memory>   // for shared_ptr
#include <optional>  // for optional
#include <string>    // for string
#include <utility>   // for pair
#include <vector>    // for vector

#include <gtk/gtk.h>  // for GtkWidget

#include "control/actions/ActionRef.h"  // for ActionRef
#include "enums/Action.enum.h"          // for Action

#include "ToolButton.h"  // for ToolButton

class ActionDatabase;

class ShapeToolButton: public ToolButton {
public:
    /**
     * @param variants Actions selecting the shape's variants, with their icons (full names). The first one is the
     *      button's own action and icon. Empty for shapes without variants.
     * @param arrowheadsAction The action holding the shape's arrowhead mode, for shapes with arrowheads
     */
    ShapeToolButton(std::string id, Category cat, Action action, std::string iconName, std::string description,
                    const ActionDatabase& db, const std::vector<std::pair<Action, std::string>>& variants,
                    std::optional<Action> arrowheadsAction);
    ~ShapeToolButton() override = default;

    struct Config {
        struct Variant {
            ActionRef gAction;
            std::string fullActionName;
            std::string icon;
        };
        std::vector<Variant> variants;
        ActionRef arrowheads;
        std::string baseIcon;
    };

protected:
    void onButtonCreated(GtkWidget* btn) override;

private:
    std::shared_ptr<const Config> config;
};
