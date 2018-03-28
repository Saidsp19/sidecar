#include "Splittable.h"

using namespace SideCar::GUI;

void
Splittable::setContents(QWidget* contents)
{
    addWidget(contents);
    setCurrentWidget(contents);
}

Splittable*
Splittable::locateChild(bool secondOne) const
{
    QSplitter* splitter = dynamic_cast<QSplitter*>(currentWidget());
    QWidget* widget = splitter->widget(secondOne ? 1 : 0);
    return dynamic_cast<Splittable*>(widget);
}

void
Splittable::split(Qt::Orientation how, QWidget* other)
{
    split(how, other, 50, 50);
}

void
Splittable::split(Qt::Orientation how, QWidget* other, int topLeftWeight, int bottomRightWeight)
{
    QWidget* original = currentWidget();
    QSplitter* splitter = splitterFactory(how);
    Q_CHECK_PTR(splitter);
    splitter->show();
    addSplitContents(splitter, original);
    addSplitContents(splitter, other);
    QList<int> sizes;
    sizes.push_back(topLeftWeight);
    sizes.push_back(bottomRightWeight);
    splitter->setSizes(sizes);
    splitter->refresh();
    setContents(splitter);
}

void
Splittable::unsplitWith(Splittable* child)
{
    setContents(child->currentWidget());
    QSplitter* splitter = dynamic_cast<QSplitter*>(child->parentWidget());
    removeWidget(splitter);
    delete splitter;
}

void
Splittable::resizeEvent(QResizeEvent* event)
{
    Super::resizeEvent(event);
    if (size().width() == 1 || size().height() == 1) { closeView(); }
}

void
Splittable::closeView()
{
    QSplitter* splitter = dynamic_cast<QSplitter*>(parentWidget());
    if (!splitter) return;
    Splittable* parent = dynamic_cast<Splittable*>(splitter->parentWidget());
    if (!parent) return;
    parent->unsplitWith(locateTwin(splitter));
}

void
Splittable::closeOtherView()
{
    QSplitter* splitter = dynamic_cast<QSplitter*>(parentWidget());
    if (!splitter) return;
    Splittable* parent = dynamic_cast<Splittable*>(splitter->parentWidget());
    if (!parent) return;
    parent->unsplitWith(this);
}

Splittable*
Splittable::locateTwin(QSplitter* splitter) const
{
    int index = splitter->indexOf(const_cast<Splittable*>(this));
    QWidget* w = splitter->widget(index == 0 ? 1 : 0);
    return dynamic_cast<Splittable*>(w);
}

QSplitter*
Splittable::splitterFactory(Qt::Orientation how)
{
    return new QSplitter(how, this);
}

Splittable*
Splittable::splittableFactory(QSplitter* parent)
{
    return new Splittable(parent);
}

void
Splittable::addSplitContents(QSplitter* parent, QWidget* contents)
{
    Splittable* splittable = splittableFactory(parent);
    Q_CHECK_PTR(splittable);
    splittable->setContents(contents);
    splittable->show();
}
