#ifndef SIDECAR_MESSAGES_RADARCONFIGFILEWATCHER_H // -*- C++ -*-
#define SIDECAR_MESSAGES_RADARCONFIGFILEWATCHER_H

#include "boost/shared_ptr.hpp"

#include "Utils/FileWatcher.h"

namespace SideCar {
namespace Configuration {

/** A file watcher for SideCar XML configuration files. Reloads the configuration when the file changes. NOTE:
    this behavior has been disabled by the overloading of start().
*/
class RadarConfigFileWatcher : public Utils::FileWatcher {
public:
    using Ref = boost::shared_ptr<RadarConfigFileWatcher>;

    static Ref Make();

    virtual ~RadarConfigFileWatcher();

    void initialize();

private:
    RadarConfigFileWatcher() : Utils::FileWatcher() {}

    void start() {}

    bool loadFile(const std::string& path);
};

} // end namespace Messages
} // end namespace SideCar

/** \file
 */

#endif
