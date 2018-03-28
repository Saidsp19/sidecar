#ifndef SIDECAR_GUI_BSCOPE_PLAYERWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_PLAYERWINDOW_H

#include "QtCore/QBasicTimer"
#include "QtCore/QList"
#include "QtGui/QImage"
#include "QtGui/QWidget"

#include "GUI/MainWindowBase.h"

class QScrollArea;

namespace Logger {
class Log;
}
namespace SideCar {
namespace GUI {
namespace BScope {

class FrameWidget;
class ImageScaler;
class PlayerPositioner;
class PlayerSettings;

class PlayerWindow : public MainWindowBase {
    Q_OBJECT
    using Super = MainWindowBase;

public:
    static Logger::Log& Log();

    PlayerWindow(const QList<QImage>& past, const QImage& blank, int shortcut);

    void saveToSettings(QSettings&);

    void restoreFromSettings(QSettings& settings);

public slots:

    void setNormalSize(const QSize& size);

    void setScale(double scale);

    void setFrameCount(int frameCount);

    void frameAdded(int activeFrames);

private slots:

    void on_actionViewZoomIn__triggered();

    void on_actionViewZoomOut__triggered();

    void on_actionShowMainWindow__triggered();

    void on_actionShowFramesWindow__triggered();

    void on_actionPresetRevert__triggered();

    void on_actionPresetSave__triggered();

    void playbackRateChanged(int msecs);

    void updateMaxSize();

    void start();

    void stop();

    void positionerValueChanged(int value);

    void imageScalerDone(const QImage& image, int index);

private:
    void updateButtons();

    void updateInfo();

    void keyReleaseEvent(QKeyEvent* event);

    void mousePressEvent(QMouseEvent* event);

    void mouseMoveEvent(QMouseEvent* event);

    void mouseReleaseEvent(QMouseEvent* event);

    void timerEvent(QTimerEvent* event);

    void showEvent(QShowEvent* event);

    void hideEvent(QHideEvent* event);

    void rescaleImage(int index, bool all);

    const QList<QImage>& past_;
    QList<QImage> scaled_;
    PlayerSettings* playerSettings_;
    QScrollArea* scrollArea_;
    FrameWidget* frame_;
    PlayerPositioner* positioner_;
    QAction* actionViewTogglePhantomCursor_;
    ImageScaler* imageScaler_;
    QBasicTimer playTimer_;
    QPoint panFrom_;
    double scale_;
    QSize normalSize_;
    QSize scaledSize_;
    int rescalingIndex_;
    bool panning_;
    bool needUpdateMaxSize_;
    bool rescalingAll_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

#endif
