#include "QtCore/QtPlugin"

#include "GUI/SquareButton.h"
#include "SquareButtonPlugin.h"

SquareButtonPlugin::SquareButtonPlugin(QObject* parent) : QObject(parent), initialized_(false)
{
    ;
}

void
SquareButtonPlugin::initialize(QDesignerFormEditorInterface*)
{
    if (initialized_) return;
    initialized_ = true;
}

bool
SquareButtonPlugin::isInitialized() const
{
    return initialized_;
}

QWidget*
SquareButtonPlugin::createWidget(QWidget* parent)
{
    return new SideCar::GUI::SquareButton(parent);
}

QString
SquareButtonPlugin::name() const
{
    return "SideCar::GUI::SquareButton";
}

QString
SquareButtonPlugin::group() const
{
    return "Lab Widgets";
}

QIcon
SquareButtonPlugin::icon() const
{
    return QIcon(":/yellow.svg");
}

QString
SquareButtonPlugin::toolTip() const
{
    return "";
}

QString
SquareButtonPlugin::whatsThis() const
{
    return "";
}

bool
SquareButtonPlugin::isContainer() const
{
    return false;
}

QString
SquareButtonPlugin::domXml() const
{
    return "<widget class=\"SideCar::GUI::SquareButton\" "
           " name=\"squareButton\">\n"
           " <property name=\"geometry\">\n"
           "  <rect>\n"
           "   <x>0</x>\n"
           "   <y>0</y>\n"
           "   <width>64</width>\n"
           "   <height>64</height>\n"
           "  </rect>\n"
           " </property>\n"
           "</widget>\n";
}

QString
SquareButtonPlugin::includeFile() const
{
    return "GUI/SquareButton.h";
}

Q_EXPORT_PLUGIN2(SquareButtonPlugin, SquareButtonPlugin)
