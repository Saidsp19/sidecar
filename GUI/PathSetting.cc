#include "QtWidgets/QFileDialog"

#include "PathSetting.h"

using namespace SideCar::GUI;

PathSetting::PathSetting(PresetManager* mgr, QLabel* viewer, QPushButton* editor, const QString& prompt,
                         const QString& types, bool global) :
    StringSetting(mgr, viewer->objectName(), viewer->text(), global),
    prompt_(prompt), types_(types)
{
    connect(editor, SIGNAL(clicked()), this, SLOT(choosePath()));
    connect(this, SIGNAL(valueChanged(const QString&)), viewer, SLOT(setText(const QString&)));
}

void
PathSetting::choosePath()
{
    QString last = getValue();
    if (last.isEmpty()) last = "/opt/sidecar/data";

    QString file = QFileDialog::getOpenFileName(0, prompt_, last, types_);
    if (!file.isEmpty()) { setValue(file); }
}
