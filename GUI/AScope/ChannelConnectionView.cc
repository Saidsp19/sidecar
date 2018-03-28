#include "QtCore/QSignalMapper"
#include "QtGui/QColorDialog"
#include "QtGui/QContextMenuEvent"
#include "QtGui/QHeaderView"
#include "QtGui/QItemDelegate"
#include "QtGui/QMenu"
#include "QtGui/QPainter"
#include "QtGui/QStyleOptionViewItemV2"

#include "GUI/LogUtils.h"

#include "ChannelConnection.h"
#include "ChannelConnectionModel.h"
#include "ChannelConnectionView.h"
#include "ChannelEditor.h"
#include "VideoChannel.h"

using namespace SideCar::GUI::AScope;

struct ItemDelegate : public QItemDelegate {
    using Super = QItemDelegate;
    ItemDelegate(QObject* parent = 0) : QItemDelegate(parent) {}
    void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

void
ItemDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QStyleOptionViewItemV2 opt(option);
    QColor c(index.data(Qt::ForegroundRole).value<QColor>());
    opt.palette.setColor(QPalette::Active, QPalette::HighlightedText, c);
    opt.palette.setColor(QPalette::Inactive, QPalette::HighlightedText, c);
    Super::paint(painter, opt, index);
}

Logger::Log&
ChannelConnectionView::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.ChannelConnectionView");
    return log_;
}

ChannelConnectionView::ChannelConnectionView(QWidget* parent) : Super(parent), contextMenu_(new QMenu(this))
{
    connect(this, SIGNAL(doubleClicked(const QModelIndex&)), SLOT(editEntry(const QModelIndex&)));

    QHeaderView* header = horizontalHeader();
#if 1
    header->setStyleSheet("QHeaderView::section {"
                          "background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop: 0 "
                          "#616161, stop: 0.5 #505050, stop: 0.6 #434343, stop: 1 #656565);"
                          "color: #CCFFCC;"
                          "border: 1px solid #6c6c6c;"
                          "}");
#endif

    header->setClickable(false);
    header->setMovable(false);
    header->setResizeMode(QHeaderView::Fixed);

    header = verticalHeader();
    header->setVisible(false);
    header->setFont(font());

    setItemDelegate(new ItemDelegate(this));

    QMenu* menu = contextMenu_;
    menu->addAction("All Visible", this, SLOT(makeAllVisible()));
    menu->addAction("All Invisible", this, SLOT(makeAllInvisible()));
    menu->addSeparator();
    menu->addAction("Remove All", this, SLOT(removeAll()));
}

void
ChannelConnectionView::makeAllVisible()
{
    ChannelConnectionModel* model = static_cast<ChannelConnectionModel*>(this->model());
    for (int row = 0; row < model->rowCount(); ++row) { model->getChannelConnection(row)->setVisible(true); }
}

void
ChannelConnectionView::makeAllInvisible()
{
    ChannelConnectionModel* model = static_cast<ChannelConnectionModel*>(this->model());
    for (int row = 0; row < model->rowCount(); ++row) { model->getChannelConnection(row)->setVisible(false); }
}

void
ChannelConnectionView::removeAll()
{
    ;
}

void
ChannelConnectionView::editEntry(const QModelIndex& index)
{
    static Logger::ProcLog log("editEntry", Log());
    QModelIndex where(index);
    LOGINFO << where.row() << '/' << where.column() << std::endl;
    if (!where.isValid()) return;

    if (where.column() == ChannelConnectionModel::kColor) {
        QColor color(model()->data(where, Qt::EditRole).value<QColor>());
        color = QColorDialog::getColor(color, static_cast<QWidget*>(parent()));
        Q_ASSERT(where.isValid());
        if (color.isValid()) model()->setData(where, color);
    }

    else if (where.column() >= ChannelConnectionModel::kName && where.column() <= ChannelConnectionModel::kVoltageMax) {
        ChannelEditor editor(window());

        ChannelConnectionModel* model = static_cast<ChannelConnectionModel*>(this->model());
        ChannelConnection* connection = model->getChannelConnection(where.row());
        editor.visible_->setChecked(connection->isVisible());
        editor.showPeakBars_->setChecked(connection->isShowingPeakBars());

        VideoChannel& channel(connection->getChannel());
        editor.sampleMin_->setValue(channel.getSampleMin());
        editor.sampleMax_->setValue(channel.getSampleMax());
        editor.voltageMin_->setValue(channel.getVoltageMin());
        editor.voltageMax_->setValue(channel.getVoltageMax());

        if (editor.exec() == QDialog::Accepted) {
            model->setData(index.sibling(where.row(), ChannelConnectionModel::kVisible), editor.visible_->isChecked());

            model->setData(index.sibling(where.row(), ChannelConnectionModel::kShowPeakBars),
                           editor.showPeakBars_->isChecked());

            channel.setSampleToVoltageScaling(editor.sampleMin_->value(), editor.sampleMax_->value(),
                                              editor.voltageMin_->value(), editor.voltageMax_->value());

            QVariant dummy;
            for (int col = ChannelConnectionModel::kSampleMin; col <= ChannelConnectionModel::kVoltageMax; ++col) {
                model->setData(index.sibling(where.row(), col), dummy);
            }

            model->saveVideoChannelSettings();
        }
    }
}

void
ChannelConnectionView::rowsInserted(const QModelIndex& parent, int start, int end)
{
    static Logger::ProcLog log("rowsInserted", Log());
    LOGINFO << start << ' ' << end << std::endl;
    scheduleDelayedItemsLayout();
    adjustColumnSizes();
    Super::rowsInserted(parent, start, end);
}

void
ChannelConnectionView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    static Logger::ProcLog log("rowsAboutToBeRemoved", Log());
    LOGINFO << start << ' ' << end << std::endl;
    Super::rowsAboutToBeRemoved(parent, start, end);
    scheduleDelayedItemsLayout();
    adjustColumnSizes();
}

void
ChannelConnectionView::resizeEvent(QResizeEvent* event)
{
    scheduleDelayedItemsLayout();
    adjustColumnSizes();
    Super::resizeEvent(event);
}

void
ChannelConnectionView::adjustColumnSizes()
{
    resizeColumnsToContents();
    int minimum = columnWidth(0);
    int available(viewport()->width());
    for (int column = 1; column < ChannelConnectionModel::kNumColumns; ++column) available -= columnWidth(column);
    if (available > minimum) setColumnWidth(0, available);
}

void
ChannelConnectionView::contextMenuEvent(QContextMenuEvent* event)
{
    if (contextMenu_) contextMenu_->exec(event->globalPos());
}
