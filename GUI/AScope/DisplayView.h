#ifndef SIDECAR_GUID_ASCOPE_DISPLAYVIEW_H // -*- C++ -*-
#define SIDECAR_GUID_ASCOPE_DISPLAYVIEW_H

#include "QtGui/QFrame"

class QSettings;

namespace Logger {
class Log;
}

namespace SideCar {
namespace GUI {
namespace AScope {

class AzimuthLatch;
class ScaleWidget;
class Visualizer;

/** Container that hosts a Visualizer widget. Uses the QFrame base class to show an indicator (frame) when the
    held Visualizer object has focus. Contains two ScaleWidget objects that provide horizontal and vertical
    scales next to the Visualizer object. Provides the Visualizer with its background QImage that shows the grid
    lines created by the ScaleWidget objects.
*/
class DisplayView : public QFrame {
    Q_OBJECT
    using Super = QFrame;

public:
    static Logger::Log& Log();

    /** Create a new view with initial sprite visibility settings taken from a configuration file.

        \param parent QWidget containing this object
    */
    DisplayView(QWidget* parent, AzimuthLatch* azimuthLatch);

    /** Destructor.
     */
    ~DisplayView();

    /** Save current view settings to a configuration settings file.

        \param settings the file to write to
    */
    void saveToSettings(QSettings& settings);

    /** Read view settings from a configuration settings file created by a previous run of the AScope program.

        \param settings file to read from
    */
    void restoreFromSettings(QSettings& settings);

    /** Duplicate the view settings of another DisplayView object.

        \param displayView the view to copy
    */
    void duplicate(const DisplayView* displayView);

    /** Obtain the held Visualizer object used to show data plots.

        \return Visualizer object
    */
    Visualizer* getVisualizer() const { return visualizer_; }

    /** Determine if the horizontal scale widget is visible.

        \return true if so
    */
    bool isHorizontalScaleVisible() const;

    /** Determine if the vertical scale widget is visible.

        \return true if so
    */
    bool isVerticalScaleVisible() const;

    /** Determine if the gridlines of the plot are shown.

        \return true if so
    */
    bool isShowingGrid() const;

    bool isFrozen() const;

    /** Determine if the gridlines of the plot are shown.

        \return true if so
    */
    bool isShowingPeakBars() const;

    /** Make this DisplayView the active DisplayView object, the one that has focus and whose channel
        connections are visible in the ChannelConnectionWindow.
    */
    void setActiveDisplayView();

signals:

    /** Notification sent out when the active DisplayView widget changes.

        \param displayView the new active widget
    */
    void activeDisplayViewChanged(DisplayView* displayView);

public slots:

    /** Action handler to adjust the visibility of the horizontal scale widget.

        \param value visibility value
    */
    void setHorizontalScaleVisibility(bool value);

    /** Action handler to adjust the visibility of the vertical scale widget.

        \param value visibility value
    */
    void setVerticalScaleVisibility(bool value);

    /** Action handler to control whether grid lines are drawn in the Visualizer background.

        \param state true if so
    */
    void setShowGrid(bool state);

    void setFrozen(bool state);

    /** Action handler to control whether grid lines are drawn in the Visualizer background.

        \param state true if so
    */
    void setShowPeakBars(bool state);

private slots:

    /** Notification from the held Visualizer object that its transform has changed. Updates the horizontal and
     * vertical scale widgets.
     */
    void visualizerTransformChanged();

    /** Notification from the held Visualizer that the cursor moved. Contains new cursor coordinates in local
        and real-world unit.

        \param local cursor position in widget coordinates

        \param world cursor position in read-world coordinates
    */
    void updateCursorPosition(const QPoint& local, const QPointF& world);

private:
    void initialize();
    void resizeEvent(QResizeEvent* event);
    void updateActiveDisplayViewIndicator(bool on);
    void makeBackground();

    Visualizer* visualizer_;
    ScaleWidget* horizontalScale_;
    ScaleWidget* verticalScale_;

    static DisplayView* activeDisplayView_;
};

} // end namespace AScope
} // namespace GUI
} // namespace SideCar

/** \file
 */

#endif
