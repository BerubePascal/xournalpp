/*
 * Xournal++
 *
 * Vertical margin line on plain and graph backgrounds (config key "ml", see BackgroundConfig). Same look as the
 * vertical line of the "ruled with vertical line" background (LinedBackgroundView).
 *
 * @license GNU GPLv2 or later
 */

#pragma once

#include <cairo.h>  // for cairo_t

#include "util/Color.h"  // for Color

#include "PlainBackgroundView.h"  // for PlainBackgroundView

class BackgroundConfig;

namespace xoj::view {

struct MarginLine {
    /// Reads the margin line from the configuration. distance <= 0 means no margin line
    MarginLine(const BackgroundConfig& config, Color backgroundColor);

    /// Draws the line at x (in page coordinates)
    void draw(cairo_t* cr, double x, double pageHeight) const;

    double distance = 0.0;  ///< Requested distance from the left edge
    Color color;
};

/// Plain background with a margin line exactly at the requested distance
class PlainMarginLineBackgroundView: public PlainBackgroundView {
public:
    PlainMarginLineBackgroundView(double pageWidth, double pageHeight, Color backgroundColor,
                                  const BackgroundConfig& config);

    void draw(cairo_t* cr) const override;

private:
    MarginLine marginLine;
};

}  // namespace xoj::view
