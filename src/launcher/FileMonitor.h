#pragma once

#include <string>
#include <vector>
#include <filesystem>

namespace fileMonitor {

struct Info {
    int notifyFd;
    std::filesystem::path path;
    int wholeDirectory = 0;
};

void start( Info & info );
bool fileUpdated( Info & info );
void stop( Info & info );

} // namespace fileMonitor
