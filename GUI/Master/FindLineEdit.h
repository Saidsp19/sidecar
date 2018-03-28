#ifndef SIDECAR_GUI_MASTER_FINDLINEEDIT_H // -*- C++ -*-
#define SIDECAR_GUI_MASTER_FINDLINEEDIT_H

#include "QtGui/QLineEdit"

class QToolButton;

namespace SideCar {
namespace GUI {
namespace Master {

class FindLineEdit : public QLineEdit {
    Q_OBJECT
public:
    FindLineEdit(QWidget* parent = 0);

protected:
    void resizeEvent(QResizeEvent* event);

private slots:

    void updateCloseButton(const QString& text);

private:
    QToolButton* findMenu_;
    QToolButton* clearButton_;
};

} // end namespace Master
} // end namespace GUI
} // end namespace SideCar

#endif
