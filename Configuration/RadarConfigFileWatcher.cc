#include "QtCore/QFile"

#include "Logger/Log.h"
#include "Utils/FilePath.h"

#include "Loader.h"
#include "RadarConfigFileWatcher.h"

using namespace SideCar::Configuration;

RadarConfigFileWatcher::Ref
RadarConfigFileWatcher::Make()
{
    return Ref(new RadarConfigFileWatcher);
}

RadarConfigFileWatcher::~RadarConfigFileWatcher()
{}

bool
RadarConfigFileWatcher::loadFile(const std::string& path)
{
    Logger::ProcLog log("loadFile", Log());
    LOGINFO << path << std::endl;

    Loader configLoader;
    if (!configLoader.load(path)) {
        LOGERROR << "failed to load file - " << configLoader.getLastLoadResult() << std::endl;
        if (configLoader.getLastLoadResult() == Configuration::Loader::kFailedXMLParse) {
            int line, col;
            auto info = configLoader.getParseErrorInfo(line, col);
            LOGERROR << "Line: " << line << " Col: " << col << " - " << qPrintable(info) << std::endl;
        }
        return false;
    }

    return true;
}
