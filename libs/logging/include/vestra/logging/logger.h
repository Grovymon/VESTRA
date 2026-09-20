#pragma once

#include <QString>

namespace vestra::logging {

class Logger final {
  public:
    static bool install(const QString& processName, QString* error = nullptr);
    static void uninstall();
    static QString currentLogFile();
};

} // namespace vestra::logging

