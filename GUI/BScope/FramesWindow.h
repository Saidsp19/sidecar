#ifndef SIDECAR_GUI_BSCOPE_FRAMESWINDOW_H // -*- C++ -*-
#define SIDECAR_GUI_BSCOPE_FRAMESWINDOW_H

#include "QtCore/QList"
#include "QtGui/QImage"
#include "QtGui/QWidget"

#include "GUI/MainWindowBase.h"

class QScrollArea;
class QVBoxLayout;

namespace Logger {
class Log;
}
namespace SideCar {
namespace GUI {
namespace BScope {

class FrameWidget;
class ImageScaler;
class FramesListSettings;
class FramesPositioner;

class FramesWindow : public MainWindowBase {
    Q_OBJECT
    using Super = MainWindowBase;

public:
    static Logger::Log& Log();

    FramesWindow(const QList<QImage>& past, int shortcut);

    void saveToSettings(QSettings&);

    void restoreFromSettings(QSettings& settings);

public slots:

    void setNormalSize(const QSize& size);

    void setScale(double scale);

    void setFrameCount(int frameCount);

    void frameAdded(int activeFrames);

    void showPhantomCursor(bool state);

private slots:

    void verticalScrollBarChanged(int value);

    void on_actionViewZoomIn__triggered();

    void on_actionViewZoomOut__triggered();

    void on_actionShowMainWindow__triggered();

    void on_actionShowPlayerWindow__triggered();

    void on_actionPresetRevert__triggered();

    void on_actionPresetSave__triggered();

    void updateMaxSize();

    void positionerValueChanged(int value);

private:
    void updateInfo();

    void keyReleaseEvent(QKeyEvent* event);

    void mousePressEvent(QMouseEvent* event);

    void mouseMoveEvent(QMouseEvent* event);

    void mouseReleaseEvent(QMouseEvent* event);

    void showEvent(QShowEvent* event);

    void hideEvent(QHideEvent* event);

    void useCursor(const QCursor& cursor);

    void rescaleImage(int index, bool all);

    const QList<QImage>& past_;
    QList<QImage> scaled_;
    FramesListSettings* framesListSettings_;
    QList<FrameWidget*> frames_;
    QScrollArea* scrollArea_;
    QWidget* framesView_;
    QVBoxLayout* framesLayout_;
    FramesPositioner* positioner_;
    QAction* actionViewTogglePhantomCursor_;
    ImageScaler* imageScaler_;
    QPoint panFrom_;
    double scale_;
    QSize normalSize_;
    QSize scaledSize_;
    int rescalingIndex_;
    bool panning_;
    bool restoreFromSettings_;
    bool needUpdateMaxSize_;
    bool positioned_;
    bool scrolled_;
    bool rescalingAll_;
};

} // end namespace BScope
} // end namespace GUI
} // end namespace SideCar

#endif
