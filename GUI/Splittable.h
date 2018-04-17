#ifndef SIDECAR_GUI_SPLITTABLE_H // -*- C++ -*-
#define SIDECAR_GUI_SPLITTABLE_H

#include "QtWidgets/QSplitter"
#include "QtWidgets/QStackedWidget"

namespace SideCar {
namespace GUI {

/** Container widget that allows its contents to be split into two widgets controlled by a QSplitter object.
    Each split component may be further split to any level. Finally, any split widget may be unsplit by moving
    the slider all the way in either direction, resulting in one of the widgets having zero size in one of its
    dimensions. The widget that was minimized is removed from the widget hierarchy along with the QSplitter
    object, and the remaining widget is left in the place where the QSplitter object was.
*/
class Splittable : public QStackedWidget {
    Q_OBJECT
    using Super = QStackedWidget;

public:
    /** Constructor.The container starts with no contents.

        \param parent parent widget for this one
    */
    Splittable(QWidget* parent) : Super(parent) {}

    /** Install a widget as the one to show for the widget representation. This widget will also become a child
        of a QSplitter if we are ever split.

        \param widget widget to use
    */
    void setContents(QWidget* widget);

    /** Obtain the widget in the top/left position of our QSplitter widget. Only valud if isSplit() returns
        true.

        \return QWidget object
    */
    Splittable* topLeft() const { return locateChild(false); }

    /** Obtain the widget in the bottom/right position of our QSplitter widget. Only valid if isSplit() returns
        true.

        \return QWidget object
    */
    Splittable* bottomRight() const { return locateChild(true); }

    /** Obtain the QSplitter object that is in use in a split view. Only valid if isSplit() returns true.

        \return QSplitter object or NULL
    */
    QSplitter* getSplitter() const { return dynamic_cast<QSplitter*>(currentWidget()); }

    /** Determine if the view shown by this widget is split.

        \return true if so.
    */
    bool isSplit() const { return getSplitter(); }

public slots:

    /** Perform the splitting of our contents. The widget that is set as our contents will become to the top or
        left part of a new QSplitter object, while user-supplied widget will be used as the bottom/right one.
        Each of these QWidget objects will be wrapped by a new Splittable object so that they too may be split.

        \param how direction in which the split will occur.

        \param other widget to use for the right or bottom part of the
        new QSplitter object.

        \param topLeftWeight how much space to allocate to the top/left widget
        in the new split view

        \param bottomRightWeight how much space to allocate to the
        bottom/right widget in the new split view
    */
    virtual void split(Qt::Orientation how, QWidget* other, int topLeftWeight, int bottomRightWeight);

    /** Perform the splitting of our contents. Same as above with 50/50 distribution of space to the two
        widgets.

        \param how direction in which the split will occur.

        \param other widget to use for the right or bottom part of the
        new QSplitter object.
    */
    virtual void split(Qt::Orientation how, QWidget* other);

    /** Restore a previously-split widget. The QSplitter hosting the split representations is removed from the
        widget stack, and the given widget is used as our representation.

        \param keeper widget to use for our representation
    */
    virtual void unsplitWith(Splittable* keeper);

    /** Restore a previously-split widget by removing ourselves. The resulting view will just contain our twin.
     */
    virtual void closeView();

    /** Restore a previously-split widget by removing our twin. The resulting view will just contain ourselves.
     */
    virtual void closeOtherView();

protected:
    /** Obtain a new QSplitter object to use when splitting our contents.

        \param how orientation of the split (Qt::Horizontal or Qt::Vertical)

        \return new QSplitter object
    */
    virtual QSplitter* splitterFactory(Qt::Orientation how);

    /** Obtain a new Splittable object to use as a wrapper for a QWidget. Only used when splitting our contents.

        \param splitter QSplitter object hosting the split representation

        \return new Splittable object
    */
    virtual Splittable* splittableFactory(QSplitter* parent);

    /** Create a new Splittable object to wrap a given QWidget and install in a QSplitter.

        \param splitter QSplitter object to host the new widget

        \param contents widget to wrap
    */
    virtual void addSplitContents(QSplitter* parent, QWidget* contents);

    /** Override of QWidget::resizeEvent(). Detect when we have been resized to zero in either dimension. If so,
        remove the split and make one of the split widgets our new representation.

        \param event description of the resizing that occured.
    */
    void resizeEvent(QResizeEvent* event);

    /** Obtain the top/left or bottom/right widget of a split representation. Only valid if isSplit() returns
        true.

        \param secondOne if true, obtain the bottom/right widget.

        \return found widget.
    */
    Splittable* locateChild(bool secondOne) const;

    /** Obtain the Splitter widget in the QSplitter widget that is not the given widget. Used in the
        resizeEvent() routine to determine the widget to keep when unsplitting our representation.

        \param splitter widget that contains the two split representations

        \return Splitter object whose contents we will keep during the unsplit
        operation.
    */
    Splittable* locateTwin(QSplitter* splitter) const;
};

} // namespace GUI
} // namespace SideCar

/** \file
 */

#endif
