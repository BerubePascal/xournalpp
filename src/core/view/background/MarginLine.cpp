#include "MarginLine.h"

#include "model/BackgroundConfig.h"  // for BackgroundConfig
#include "util/Util.h"               // for cairo_set_source_rgbi

using namespace background_config_strings;
using namespace xoj::view;

namespace {
// Same look as the vertical line of the "ruled with vertical line" background (see LinedBackgroundView and
// RuledBackgroundView)
constexpr Color DEFAULT_COLOR = Colors::xopp_deeppink;
constexpr Color ALT_DEFAULT_COLOR = Colors::xopp_midnightblue;  // On dark backgrounds
constexpr double LINE_WIDTH = 0.5;

auto colorOr(const BackgroundConfig& config, const char* key, Color defaultColor) -> Color {
    if (uint32_t hexColor; config.loadValueHex(key, hexColor)) {
        return Color(hexColor);
    }
    return defaultColor;
}
}  // namespace

MarginLine::MarginLine(const BackgroundConfig& config, Color backgroundColor) {
    config.loadValue(CFG_MARGIN_LINE, distance);
    color = backgroundColor.isLight() ? colorOr(config, CFG_FOREGROUND_COLOR_2, DEFAULT_COLOR) :
                                        colorOr(config, CFG_ALT_FOREGROUND_COLOR_2, ALT_DEFAULT_COLOR);
}

void MarginLine::draw(cairo_t* cr, double x, double pageHeight) const {
    cairo_save(cr);
    Util::cairo_set_source_rgbi(cr, color);
    cairo_set_line_width(cr, LINE_WIDTH);
    cairo_set_line_cap(cr, CAIRO_LINE_CAP_BUTT);
    cairo_move_to(cr, x, 0);
    cairo_line_to(cr, x, pageHeight);
    cairo_stroke(cr);
    cairo_restore(cr);
}

PlainMarginLineBackgroundView::PlainMarginLineBackgroundView(double pageWidth, double pageHeight,
                                                             Color backgroundColor, const BackgroundConfig& config):
        PlainBackgroundView(pageWidth, pageHeight, backgroundColor), marginLine(config, backgroundColor) {}

void PlainMarginLineBackgroundView::draw(cairo_t* cr) const {
    PlainBackgroundView::draw(cr);
    if (marginLine.distance > 0 && marginLine.distance < pageWidth) {
        marginLine.draw(cr, marginLine.distance, pageHeight);
    }
}
