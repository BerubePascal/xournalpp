#include "BackgroundSpacing.h"

#include <array>   // for array
#include <string>  // for string

#include <glib.h>  // for g_ascii_formatd

#include "model/BackgroundConfig.h"  // for BackgroundConfig, CFG_RASTER
#include "util/Util.h"               // for PRECISION_FORMAT_STRING

using namespace background_config_strings;

namespace {
/// Default spacing of each kind of background (see the background views)
auto defaultSpacing(PageTypeFormat format) -> std::optional<double> {
    switch (format) {
        case PageTypeFormat::Ruled:
        case PageTypeFormat::Lined:
            return 24.0;
        case PageTypeFormat::Graph:
        case PageTypeFormat::Dotted:
        case PageTypeFormat::IsoDotted:
        case PageTypeFormat::IsoGraph:
            return 14.17;  // 5 mm
        default:
            return std::nullopt;
    }
}
}  // namespace

auto BackgroundSpacing::get(const PageType& pageType) -> std::optional<double> {
    auto spacing = defaultSpacing(pageType.format);
    if (spacing) {
        BackgroundConfig(pageType.config).loadValue(CFG_RASTER, *spacing);
    }
    return spacing;
}

auto BackgroundSpacing::with(const PageType& pageType, double spacing) -> PageType {
    std::array<char, G_ASCII_DTOSTR_BUF_SIZE> value{};
    g_ascii_formatd(value.data(), value.size(), Util::PRECISION_FORMAT_STRING, spacing);
    const std::string entry = std::string(CFG_RASTER) + "=" + value.data();

    // The configuration is a list of key=value separated by commas: replace the spacing, or add it
    std::string config;
    bool replaced = false;
    size_t start = 0;
    while (start <= pageType.config.size() && !pageType.config.empty()) {
        size_t end = pageType.config.find(',', start);
        if (end == std::string::npos) {
            end = pageType.config.size();
        }
        std::string item = pageType.config.substr(start, end - start);
        if (item.rfind(std::string(CFG_RASTER) + "=", 0) == 0) {
            item = entry;
            replaced = true;
        }
        if (!item.empty()) {
            config += (config.empty() ? "" : ",") + item;
        }
        start = end + 1;
    }
    if (!replaced) {
        config += (config.empty() ? "" : ",") + entry;
    }

    PageType res = pageType;
    res.config = config;
    return res;
}
