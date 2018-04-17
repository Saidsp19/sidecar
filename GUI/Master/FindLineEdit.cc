#include "QtWidgets/QStyle"
#include "QtWidgets/QToolButton"

#include "FindLineEdit.h"

using namespace SideCar::GUI::Master;

FindLineEdit::FindLineEdit(QWidget* parent) :
    QLineEdit(parent), findMenu_(new QToolButton(this)), clearButton_(new QToolButton(this))
{
    QPixmap findPixmap(":/findMenu.png");
    findMenu_->setIcon(QIcon(findPixmap));
    findMenu_->setIconSize(QSize(16, 16));
    findMenu_->setCursor(Qt::ArrowCursor);
    findMenu_->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    clearButton_->hide();

    QPixmap clearPixmap(":/clearFind.png");
    clearButton_->setIcon(QIcon(clearPixmap));
    clearButton_->setIconSize(QSize(16, 16));
    clearButton_->setCursor(Qt::ArrowCursor);
    clearButton_->setStyleSheet("QToolButton { border: none; padding: 0px; }");
    clearButton_->hide();

    connect(clearButton_, SIGNAL(clicked()), SLOT(clear()));
    connect(this, SIGNAL(textChanged(const QString&)), SLOT(updateCloseButton(const QString&)));

    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    setStyleSheet(QString("QLineEdit { padding-left: %1px; padding-right: %2px }")
                      .arg(findMenu_->sizeHint().width() + frameWidth + 1)
                      .arg(clearButton_->sizeHint().width() + frameWidth + 1));

    QSize msz = minimumSizeHint();
    setMinimumSize(qMax(msz.width(), clearButton_->sizeHint().height() + frameWidth * 2 + 2),
                   qMax(msz.height(), clearButton_->sizeHint().height() + frameWidth * 2 + 2));
}

void
FindLineEdit::resizeEvent(QResizeEvent* event)
{
    QSize sz = clearButton_->sizeHint();
    int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
    findMenu_->move(0, (rect().bottom() + 1 - sz.height()) / 2);
    clearButton_->move(rect().right() - frameWidth - sz.width(), (rect().bottom() + 1 - sz.height()) / 2);
}

void
FindLineEdit::updateCloseButton(const QString& text)
{
    clearButton_->setVisible(!text.isEmpty());
}
