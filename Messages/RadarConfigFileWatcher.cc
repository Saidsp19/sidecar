#include "QtCore/QFile"
#include "QtXml/QDomDocument"

#include "Logger/Log.h"

#include "RadarConfig.h"
#include "RadarConfigFileWatcher.h"

using namespace SideCar::Messages;

bool
RadarConfigFileWatcher::loadFile(const std::string& path)
{
    Logger::ProcLog log("doLoad", Log());
    LOGINFO << path << std::endl;

    QDomDocument doc;
    QFile file(path.c_str());
    if (! file.open(QIODevice::ReadOnly)) {
	LOGERROR << "failed to open file " << path << std::endl;
	return false;
    }

    if (! doc.setContent(&file)) {
	LOGERROR << "failed to parse configuration file " << path << std::endl;
	return false;
    }

    QDomNode radar(doc.elementsByTagName("radar").at(0));
    if (! RadarConfig::Load(radar.toElement())) {
	LOGERROR << "invalid configuration file " << path << std::endl;
	return false;
    }

    return true;
}
