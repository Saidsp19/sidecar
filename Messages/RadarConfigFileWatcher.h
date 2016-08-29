#ifndef SIDECAR_MESSAGES_RADARCONFIGFILEWATCHER_H // -*- C++ -*-
#define SIDECAR_MESSAGES_RADARCONFIGFILEWATCHER_H

#include "Utils/FileWatcher.h"

namespace SideCar {
namespace Messages {

/** A file watcher for SideCar XML configuration files. Reloads the configuration when the file changes. NOTE:
    this behavior has been disabled by the overloading of start().
*/
class RadarConfigFileWatcher : public Utils::FileWatcher
{
public:
    RadarConfigFileWatcher() : Utils::FileWatcher() {}

private:
    void start() {}

    bool loadFile(const std::string& path);
};

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
