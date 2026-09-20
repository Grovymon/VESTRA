#pragma once

#include <QString>

namespace vestra::core {

class ApplicationPaths final {
  public:
    static QString root();
    static QString config();
    static QString cache();
    static QString recovery();
    static QString logs();
    static QString temp();
    static bool ensureCreated(QString* error = nullptr);
};

} // namespace vestra::core

