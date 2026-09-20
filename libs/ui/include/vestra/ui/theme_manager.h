#pragma once

#include "vestra/settings/app_settings.h"

namespace vestra::ui {

class ThemeManager final {
  public:
    static void apply(settings::Theme theme);
};

} // namespace vestra::ui

