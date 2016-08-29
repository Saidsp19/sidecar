#ifndef SIDECAR_GUI_BUGPLOTEMITTERSETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_BUGPLOTEMITTERSETTINGS_H

#include "GUI/OnOffSettingsBlock.h"
#include "GUI/StringSetting.h"
#include "Messages/BugPlot.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {

class ChannelSetting;
class UDPMessageWriter;

class BugPlotEmitterSettings : public OnOffSettingsBlock
{
    Q_OBJECT
    using Super = OnOffSettingsBlock;
public:

    static Logger::Log& Log();

    BugPlotEmitterSettings(BoolSetting* enabled, StringSetting* serviceName, StringSetting* address,
                           ChannelSetting* channel);

    ~BugPlotEmitterSettings();

    const QString& getServiceName() const { return serviceName_->getValue(); }

    const QString& getAddress() const { return address_->getValue(); }

    Messages::BugPlot::Ref addBugPlot(double range, double azimuth, double elevation = 0.0);

private slots:

    void updateWriter();
    void needUpdate();

private:

    void addWriter();
    void removeWriter();

    StringSetting* serviceName_;
    StringSetting* address_;
    ChannelSetting* channel_;
    UDPMessageWriter* writer_;
    QString localHostName_;
    uint16_t sequenceCounter_;
    bool needUpdate_;
};

} // end namespace GUI
} // end namespace SideCar

#endif
