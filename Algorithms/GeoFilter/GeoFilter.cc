#include <algorithm>  // for std::transform
#include <functional> // for std::bind* and std::mem_fun*

#include "Algorithms/Controller.h"
#include "GUI/LogUtils.h"
#include "IO/StateEmitter.h"
#include "Utils/Utils.h"

#include "GeoFilter.h"
#include "GeoFilter_defaults.h"

#include "QtCore/QFile"
#include "QtCore/QString"
#include "QtCore/QStringList"
#include "QtXml/QDomDocument"
#include "QtXml/QDomElement"

using namespace SideCar;
using namespace SideCar::Algorithms;

struct Filter {
    Filter() :
        name(), rangeMin(0.0), rangeMax(0.0), azMin(0.0), azMax(0.0), attenuation(1.0), offset(0.0),
        clampMin(std::numeric_limits<short>::min()), clampMax(std::numeric_limits<short>::max())
    {
    }
    QString name;
    double rangeMin;
    double rangeMax;
    double azMin;
    double azMax;
    double attenuation;
    double offset;
    double clampMin;
    double clampMax;
};

struct GeoFilter::Private {
    Private() : filters_(), stateEmitter_() {}
    std::vector<Filter> filters_;
    IO::StateEmitter stateEmitter_;
};

// Constructor. Do minimal initialization here. Registration of processors and runtime parameters should occur in the
// startup() method. NOTE: it is WRONG to call any virtual functions here...
//
GeoFilter::GeoFilter(Controller& controller, Logger::Log& log) :
    Super(controller, log), enabled_(Parameter::BoolValue::Make("enabled", "Enabled", kDefaultEnabled)),
    configPath_(Parameter::ReadPathValue::Make("configPath", "Config Path", kDefaultConfigPath)),
    load_(Parameter::NotificationValue::Make("load", "Load Configuration", 0)), p_(new Private)
{
    load_->connectChangedSignalTo(boost::bind(&GeoFilter::loadNotification, this, _1));
}

void
GeoFilter::loadNotification(const Parameter::NotificationValue& value)
{
    Logger::ProcLog log("loadNotification", getLog());
    loadConfig();
}

// Startup routine. This is called right after the Controller loads our DLL and creates an instance of the GeoFilter
// class. Place registerProcessor and registerParameter calls here. Also, be sure to invoke Algorithm::startup() as
// shown below.
//
bool
GeoFilter::startup()
{
    registerProcessor<GeoFilter, Messages::Video>(&GeoFilter::processInput);
    return p_->stateEmitter_.open(getController().getTaskName()) && registerParameter(enabled_) &&
           registerParameter(configPath_) && registerParameter(load_) && Super::startup();
}

bool
GeoFilter::shutdown()
{
    p_->stateEmitter_.close();
    return true;
}

bool
GeoFilter::reset()
{
    // The algorithm is transitioning from a stop state to a run state. Attempt to load our configuration if we have
    // not already done so.
    //
    if (p_->filters_.empty()) return loadConfig();
    return true;
}

bool
GeoFilter::loadConfig()
{
    p_->filters_.clear();
    std::string path = configPath_->getValue();
    if (path.size()) {
        if (!loadConfigFile(path)) {
            getController().setError("Failed to load configuration file");
            return false;
        }
        getController().clearError();
    }

    return true;
}

bool
GeoFilter::loadConfigFile(const std::string& path)
{
    Logger::ProcLog log("loadConfig", getLog());

    QFile file(QString::fromStdString(path));
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        LOGERROR << "failed to open file '" << path << "'" << std::endl;
        return false;
    }

    QString errorText;
    int errorLine;
    int errorColumn;
    QDomDocument dom;
    if (!dom.setContent(&file, false, &errorText, &errorLine, &errorColumn)) {
        LOGERROR << "failed to parse file: line " << errorLine << " - " << errorText << std::endl;
        return false;
    }

    static const QString kRootEntity("geofilter");
    static const QString kFilterEntity("filter");

    QDomElement top = dom.namedItem(kRootEntity).toElement();
    if (top.isNull()) {
        LOGERROR << "missing <geofilter> root entity" << std::endl;
        return false;
    }

    static const QRegExp trueRE("(t(rue)?)|(y(es)?)|1", Qt::CaseInsensitive);
    static const QRegExp falseRE("(f(alse)?)|(n(o)?)|0", Qt::CaseInsensitive);

    QDomElement filterSpec = top.firstChildElement(kFilterEntity);
    while (!filterSpec.isNull()) {
        Filter filter;
        bool enabled;

        filter.name = filterSpec.attribute("name", "").trimmed();
        if (filter.name.isEmpty()) {
            LOGERROR << "missing name attribute" << std::endl;
            return false;
        }

        QString value = filterSpec.attribute("enabled", "true").trimmed();
        if (trueRE.exactMatch(value)) {
            enabled = true;
        } else if (falseRE.exactMatch(value)) {
            enabled = false;
        } else {
            LOGERROR << "invalid/missing enabled attribute value - " << value << std::endl;
            return false;
        }

        QDomNodeList children = filterSpec.childNodes();
        for (int index = 0; index < children.count(); ++index) {
            QDomElement element(children.item(index).toElement());
            if (element.isNull()) {
                LOGERROR << "expected filter entity" << std::endl;
                return false;
            }

            QString tag = element.tagName();
            if (tag == "range") {
                QStringList values = element.text().split(' ', QString::SkipEmptyParts);
                if (values.size() != 2) {
                    LOGERROR << "invalid range values" << std::endl;
                    return false;
                }
                bool ok = false;
                filter.rangeMin = values[0].toDouble(&ok);
                if (!ok) {
                    LOGERROR << "invalid range min value" << std::endl;
                    return false;
                }
                ok = false;
                filter.rangeMax = values[1].toDouble(&ok);
                if (!ok) {
                    LOGERROR << "invalid range max value" << std::endl;
                    return false;
                }
            } else if (tag == "azimuth") {
                QStringList values = element.text().split(' ', QString::SkipEmptyParts);
                if (values.size() != 2) {
                    LOGERROR << "invalid azimuth values" << std::endl;
                    return false;
                }
                bool ok = false;
                filter.azMin = Utils::normalizeRadians(Utils::degreesToRadians(values[0].toDouble(&ok)));
                if (!ok) {
                    LOGERROR << "invalid azimuth min value" << std::endl;
                    return false;
                }
                ok = false;
                filter.azMax = Utils::normalizeRadians(Utils::degreesToRadians(values[1].toDouble(&ok)));
                if (!ok) {
                    LOGERROR << "invalid azimuth max value" << std::endl;
                    return false;
                }
            } else if (tag == "attenuation") {
                bool ok = false;
                filter.attenuation = element.text().toDouble(&ok);
                if (!ok) {
                    LOGERROR << "invalid attenuation value" << std::endl;
                    return false;
                }
            } else if (tag == "offset") {
                bool ok = false;
                filter.offset = element.text().toDouble(&ok);
                if (!ok) {
                    LOGERROR << "invalid offset value" << std::endl;
                    return false;
                }
            } else if (tag == "clamp") {
                QStringList values = element.text().split(' ', QString::SkipEmptyParts);
                if (values.size() != 2) {
                    LOGERROR << "invalid clamp values" << std::endl;
                    return false;
                }
                bool ok = false;
                filter.clampMin = values[0].toDouble(&ok);
                if (!ok) {
                    LOGERROR << "invalid clamp min value" << std::endl;
                    return false;
                }
                ok = false;
                filter.clampMax = values[1].toDouble(&ok);
                if (!ok) {
                    LOGERROR << "invalid clamp max value" << std::endl;
                    return false;
                }
            } else {
                LOGERROR << "unknown entity - " << tag << std::endl;
                return false;
            }
        }

        if (enabled) {
            LOGWARNING << "adding filter - " << filter.name << " az: " << Utils::radiansToDegrees(filter.azMin) << '/'
                       << Utils::radiansToDegrees(filter.azMax) << " range: " << filter.rangeMin << '/'
                       << filter.rangeMax << " attenuation: " << filter.attenuation << " offset: " << filter.offset
                       << " clamp: " << filter.clampMin << '/' << filter.clampMax << std::endl;
            p_->filters_.push_back(filter);
        }

        filterSpec = filterSpec.nextSiblingElement("filter");
    }

    return true;
}

bool
GeoFilter::processInput(const Messages::Video::Ref& inMsg)
{
    static Logger::ProcLog log("processInput", getLog());

    // Create a new message to hold the output of what we do. Note that although we pass in the input message, the new
    // message does not contain any data.
    //
    Messages::Video::Ref outMsg(Messages::Video::Make("GeoFilter", inMsg));
    outMsg->getData() = inMsg->getData();
    if (!enabled_->getValue()) return send(outMsg);

    double azimuth = inMsg->getAzimuthStart();

    const std::vector<Filter>& filters(p_->filters_);
    for (size_t index = 0; index < filters.size(); ++index) {
        const Filter& filter(filters[index]);
        if (filter.azMin <= azimuth && azimuth <= filter.azMax) {
            LOGDEBUG << "applying filter " << filter.name << std::endl;

            double rangeMin = inMsg->getRangeMin();
            double rangeFactor = inMsg->getRangeFactor();

            double offset = (filter.rangeMin - rangeMin) / rangeFactor;
            if (offset < 0.0) offset = 0.0;

            LOGDEBUG << "begin offset: " << offset << std::endl;

            Messages::Video::const_iterator in = inMsg->begin() + size_t(offset);
            Messages::Video::iterator out = outMsg->begin() + size_t(offset);

            offset = (filter.rangeMax - rangeMin) / rangeFactor;
            if (offset < 0.0)
                offset = 0.0;
            else if (offset > inMsg->size())
                offset = inMsg->size();

            LOGDEBUG << "end offset: " << offset << std::endl;

            Messages::Video::const_iterator end = inMsg->begin() + size_t(offset);

            while (in < end) {
                double v = *in++ * filter.attenuation + filter.offset;
                if (v < filter.clampMin) v = filter.clampMin;
                if (v > filter.clampMax) v = filter.clampMax;
                *out++ = ::rint(v);
            }
        }
    }

    bool rc = send(outMsg);
    LOGDEBUG << "rc: " << rc << std::endl;
    return rc;
}

void
GeoFilter::setInfoSlots(IO::StatusBase& status)
{
    status.setSlot(kEnabled, enabled_->getValue());
    status.setSlot(kActiveFilterCount, int(p_->filters_.size()));
    status.setSlot(kConfigPath, configPath_->getValue());
}

extern "C" ACE_Svc_Export void*
FormatInfo(const IO::StatusBase& status, int role)
{
    if (role != Qt::DisplayRole) return NULL;
    if (!status[GeoFilter::kEnabled]) return Algorithm::FormatInfoValue("Disabled");

    // Format status information here.
    //
    QString configPath(QString::fromStdString(status[GeoFilter::kConfigPath]));
    int filterCount = status[GeoFilter::kActiveFilterCount];
    return Algorithm::FormatInfoValue(QString("Active: %1  Path: %2").arg(filterCount).arg(configPath));
}

// Factory function for the DLL that will create a new instance of the GeoFilter class. DO NOT CHANGE!
//
extern "C" ACE_Svc_Export Algorithm*
GeoFilterMake(Controller& controller, Logger::Log& log)
{
    return new GeoFilter(controller, log);
}
