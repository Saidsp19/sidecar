#ifndef SIDECAR_GUI_ASCOPE_VIEWCHANGER_H // -*- C++ -*-
#define SIDECAR_GUI_ASCOPE_VIEWCHANGER_H

#include "QtGui/QRubberBand"

namespace SideCar {
namespace GUI {
namespace AScope {

class Visualizer;

/** Abstract base class that manages user events associated with changing a Visualizer view transformation
    matrix using the mouse. Derived classes exist for panning and zooming changes.
*/
class ViewChanger {
public:
    /** Constructor.

        \param plotter the Visualizer to manipulate

        \param start mouse position at the start of the change in local
        coordinates
    */
    ViewChanger(Visualizer* plotter, const QPoint& start);

    /** Destructor.
     */
    virtual ~ViewChanger() {}

    /** Notification of a mouse change. Derived classes must define.

        \param pos new mouse position in local coordinates
    */
    virtual void mouseMoved(const QPoint& pos) = 0;

    /** Notification that the change is done. Derived classes must define.

        \param pos last mouse position in local coordinates
    */
    virtual void finished(const QPoint& pos) = 0;

protected:
    Visualizer* visualizer_;
    QPoint start_;
};

/** View changer for panning. Shifts the view based on changes in mouse position.
 */
class PanningViewChanger : public ViewChanger {
public:
    /** Constructor.

        \param plotter the Visualizer to manipulate

        \param start mouse position at the start of the change in local
        coordinates
    */
    PanningViewChanger(Visualizer* plotter, const QPoint& start);

    /** Destructor. If the visualizer_ attribute is non-NULL, removes the top entry in its view stack, thus
        undoing any panning that took place.
    */
    ~PanningViewChanger();

    /** Pan the Visualizer by the delta of the given mouse position and the last one.

        \param pos latest mouse position
    */
    void mouseMoved(const QPoint& pos);

    /** Notification that the change event is done. Forgets the Visualizer object so that the destructor does
        not affect its view stack.

        \param pos last mouse position.
    */
    void finished(const QPoint& pos);
};

/** View changer for zooming. Draws a 'bubber-band' rectangle that indicates the area the view changer will zoom
    into once the mouse button is released.
*/
class ZoomingViewChanger : public ViewChanger {
public:
    /** Constructor. Starts showing a QRubberBand object to indicate the region to zoom into.

        \param plotter the Visualizer to manipulate

        \param start mouse position at the start of the change in local
        coordinates
    */
    ZoomingViewChanger(Visualizer* plotter, const QPoint& start);

    /** Updates the rubber-band rectangle

        \param pos latest mouse position
    */
    void mouseMoved(const QPoint& pos);

    /** Removes the rubber-band rectangle, and creates and installs a new Visualizer::ViewRect with the zoomed
        view.

        \param pos last mouse position
    */
    void finished(const QPoint& pos);

private:
    QRubberBand rubberBand_;
};

} // end namespace AScope
} // end namespace GUI
} // end namespace SideCar

/** \file
 */

#endif
