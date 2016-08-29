#ifndef SIDECAR_GUI_PPIDISPLAY_BACKGROUNDIMAGESETTINGS_H // -*- C++ -*-
#define SIDECAR_GUI_PPIDISPLAY_BACKGROUNDIMAGESETTINGS_H

#include "QtGui/QImage"

#include "GUI/OnOffSettingsBlock.h"
#include "GUI/OpacitySetting.h"
#include "GUI/PathSetting.h"

namespace Logger { class Log; }

namespace SideCar {
namespace GUI {
namespace PPIDisplay {

/** Collection of Setting objects that control the display of a background image in a PPIWidget widget. Relies
    on the Qt QImage class to do most of the work. The QImage class supports a variety of image formats,
    including JPEG, PNG, and XPM.
*/
class BackgroundImageSettings : public OnOffSettingsBlock
{
    Q_OBJECT
    using Super = OnOffSettingsBlock;
public:

    static Logger::Log& Log();
    
    /** Constructor.

        \param enabled 

        \param path 

        \param opacity 
    */
    BackgroundImageSettings(BoolSetting* enabled, PathSetting* path,
                            OpacitySetting* opacity);

    /** Obtain the current image file path. May be empty if not set.

        \return path
    */
    QString getPath() const { return path_->getValue(); }

    /** Obtain the current image opacity value.

        \return opacity value in range [0.0-1.0].
    */
    double getOpacity() const { return opacity_->getValue(); }

    /** Obtain the filtered background image. This is the last image emitted in in the imageChanged signal.

        \return QImage reference
    */
    const QImage& getFiltered() const { return filtered_; }

signals:

    /** Notification emitted when the background image changes.

        \param image new background image
    */
    void imageChanged(const QImage& image);

private slots:

    /** Notification handler invoked when the image file path changes. Attempts to load in the new file as a
        QImage. Emits the imageChanged() signal.

        \param path 
    */
    void pathChanged(const QString& path);

    /** Create a new background image with a given opacity setting. Emits the imageChanged() signal.

        \param opacity the opacity of the resulting image
    */
    void update(double opacity);

    /** Detect when the enabled state changes, and emit the imageChanged() signal with the last generated image
        (stored in the filtered_ attribute).

        \param state new enabled state
    */
    void detectEnabledChanged(bool state);

private:
    PathSetting* path_;
    OpacitySetting* opacity_;
    QImage image_;
    QImage filtered_;
};

} // end namespace PPIDisplay
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
