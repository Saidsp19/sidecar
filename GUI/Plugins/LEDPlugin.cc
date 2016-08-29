#include "QtCore/QtPlugin"

#include "GUI/LED.h"
#include "LEDPlugin.h"

LEDPlugin::LEDPlugin(QObject *parent)
    : QObject(parent), initialized_(false)
{
    ;
}

void
LEDPlugin::initialize(QDesignerFormEditorInterface*)
{
    if (initialized_) return;
    initialized_ = true;
}

bool
LEDPlugin::isInitialized() const
{
    return initialized_;
}

QWidget*
LEDPlugin::createWidget(QWidget* parent)
{
    return new SideCar::GUI::LED(parent);
}

QString
LEDPlugin::name() const
{
    return "SideCar::GUI::LED";
}

QString
LEDPlugin::group() const
{
    return "Lab Widgets";
}

QIcon
LEDPlugin::icon() const
{
    return QIcon(":/red.svg");
}

QString
LEDPlugin::toolTip() const
{
    return "";
}

QString
LEDPlugin::whatsThis() const
{
    return "";
}

bool
LEDPlugin::isContainer() const
{
    return false;
}

QString
LEDPlugin::domXml() const
{
    return "<widget class=\"SideCar::GUI::LED\" name=\"led\">\n"
	" <property name=\"geometry\">\n"
	"  <rect>\n"
	"   <x>0</x>\n"
	"   <y>0</y>\n"
	"   <width>32</width>\n"
	"   <height>32</height>\n"
	"  </rect>\n"
	" </property>\n"
	" <property name=\"color\">\n"
	" <enum>SideCar::GUI::LED::kRed</enum>\n"
	" </property>\n"
	"</widget>\n";
}

QString
LEDPlugin::includeFile() const
{
    return "GUI/LED.h";
}

Q_EXPORT_PLUGIN2(LEDPlugin, LEDPlugin)
