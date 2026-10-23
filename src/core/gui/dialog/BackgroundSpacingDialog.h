/*
 * Xournal++
 *
 * Dialog to change the spacing of the lines / grid of the page background, on the current page or all pages
 *
 * @license GNU GPLv2 or later
 */

#pragma once

class Control;

namespace BackgroundSpacingDialog {
/// Shows the dialog (non blocking); the change is applied when the user clicks Apply
void show(Control* control);
}  // namespace BackgroundSpacingDialog
