#include "QtCore/QtPlugin"

#include "GUI/Potentiometer.h"
#include "PotentiometerPlugin.h"

using namespace SideCar::GUI;

PotentiometerPlugin::PotentiometerPlugin(QObject* parent) : QObject(parent), initialized_(false)
{
    ;
}

void
PotentiometerPlugin::initialize(QDesignerFormEditorInterface*)
{
    if (initialized_) return;
    initialized_ = true;
}

bool
PotentiometerPlugin::isInitialized() const
{
    return initialized_;
}

QWidget*
PotentiometerPlugin::createWidget(QWidget* parent)
{
    return new Potentiometer(parent);
}

QString
PotentiometerPlugin::name() const
{
    return "SideCar::GUI::Potentiometer";
}

QString
PotentiometerPlugin::group() const
{
    return "Lab Widgets";
}

QIcon
PotentiometerPlugin::icon() const
{
    return QIcon(":PotentiometerPlugin.png");
}

QString
PotentiometerPlugin::toolTip() const
{
    return "";
}

QString
PotentiometerPlugin::whatsThis() const
{
    return "";
}

bool
PotentiometerPlugin::isContainer() const
{
    return false;
}

QString
PotentiometerPlugin::domXml() const
{
    return "<widget class=\"SideCar::GUI::Potentiometer\" name=\"pot\">\n"
           " <property name=\"geometry\">\n"
           "  <rect>\n"
           "   <x>0</x>\n"
           "   <y>0</y>\n"
           "   <width>200</width>\n"
           "   <height>200</height>\n"
           "  </rect>\n"
           " </property>\n"
           "</widget>\n";
}

QString
PotentiometerPlugin::includeFile() const
{
    return "GUI/Potentiometer.h";
}

Q_EXPORT_PLUGIN2(PotentiometerPlugin, PotentiometerPlugin)
