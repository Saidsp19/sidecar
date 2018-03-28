#include "QtGui/QKeyEvent"

#include "GUI/LogUtils.h"
#include "GUI/ServiceEntry.h"
#include "GUI/modeltest.h"

#include "AppBase.h"
#include "ChannelConnectionModel.h"
#include "ChannelConnectionView.h"
#include "ChannelConnectionWindow.h"
#include "DisplayView.h"
#include "VideoChannel.h"

using namespace SideCar::GUI::AScope;
using namespace SideCar::Messages;

Logger::Log&
ChannelConnectionWindow::Log()
{
    static Logger::Log& log_ = Logger::Log::Find("ascope.ChannelConnectionWindow");
    return log_;
}

ChannelConnectionWindow::ChannelConnectionWindow(int shortcut) :
    ToolWindowBase("ChannelConnectionWindow", "Channels", shortcut), Ui::ChannelConnectionWindow(), model_(0)
{
    setupUi(this);
    model_ = new ChannelConnectionModel(this);
#ifdef __DEBUG__
    new ModelTest(model_, this);
#endif
    unconnected_->setEnabled(false);
    add_->setEnabled(false);
    remove_->setEnabled(false);
    moveUp_->setEnabled(false);
    moveDown_->setEnabled(false);

    connections_->setEnabled(false);
    connections_->setModel(model_);
    connect(connections_->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
            SLOT(updateWidgets()));
    connect(model_, SIGNAL(rowsInserted(const QModelIndex&, int, int)), SLOT(updateWidgets()));
    connect(model_, SIGNAL(rowsRemoved(const QModelIndex&, int, int)), SLOT(updateWidgets()));
}

void
ChannelConnectionWindow::updateWidgets()
{
    QItemSelection selection = connections_->selectionModel()->selection();
    bool enable = !selection.empty();
    connections_->setEnabled(model_->rowCount() > 0);
    remove_->setEnabled(enable);
    moveUp_->setEnabled(enable && selection.front().top() > 0);
    moveDown_->setEnabled(enable && selection.back().bottom() < model_->rowCount() - 1);
    enable = unconnected_->count() > 0;
    unconnected_->setEnabled(enable);
    add_->setEnabled(enable);
}

void
ChannelConnectionWindow::setUnconnected(const QStringList& names)
{
    QString current(unconnected_->currentText());
    unconnected_->clear();
    unconnected_->addItems(names);
    bool enable = !names.empty();
    unconnected_->setEnabled(enable);
    add_->setEnabled(enable);
    int index = unconnected_->findText(current);
    if (index != -1) unconnected_->setCurrentIndex(index);
}

QStringList
ChannelConnectionWindow::getUnconnected() const
{
    QStringList names;
    for (int index = 0; index < unconnected_->count(); ++index) names.append(unconnected_->itemText(index));
    return names;
}

VideoChannel*
ChannelConnectionWindow::getVideoChannel(const QString& channelName)
{
    return model_ ? model_->getVideoChannel(channelName) : 0;
}

ChannelConnection*
ChannelConnectionWindow::getSelectedChannelConnection() const
{
    QItemSelectionModel* selectionModel = connections_->selectionModel();
    if (!selectionModel->hasSelection()) return 0;
    QItemSelection selection = selectionModel->selection();
    QModelIndex index = selection.front().topLeft();
    return model_->getChannelConnection(index.row());
}

void
ChannelConnectionWindow::on_add__clicked()
{
    static Logger::ProcLog log("on_add__clicked", Log());
    LOGINFO << std::endl;
    if (!model_->makeConnection(unconnected_->currentText())) {
        LOGERROR << "failed to create new connect" << std::endl;
        return;
    }

    unconnected_->removeItem(unconnected_->currentIndex());
    if (unconnected_->count() == 0) {
        add_->setEnabled(false);
        unconnected_->setEnabled(false);
    }

    QModelIndex index = model_->index(model_->rowCount() - 1, 0);
    connections_->setCurrentIndex(index);
    QItemSelectionModel* selectionModel = connections_->selectionModel();
    selectionModel->select(index, QItemSelectionModel::Clear);
    selectionModel->select(index, QItemSelectionModel::Select | QItemSelectionModel::Rows);
    connections_->setFocus(Qt::OtherFocusReason);
}

void
ChannelConnectionWindow::on_remove__clicked()
{
    static Logger::ProcLog log("on_remove__clicked", Log());
    LOGINFO << std::endl;
    QItemSelectionModel* selectionModel = connections_->selectionModel();
    QItemSelection selection = selectionModel->selection();
    QModelIndex index = selection.front().topLeft();
    ServiceEntry* serviceEntry = model_->removeConnection(index.row());
    if (serviceEntry) {
        QStringList unconnected(getUnconnected());
        unconnected.append(serviceEntry->getName());
        qSort(unconnected);
        setUnconnected(unconnected);
    }

    if (!selectionModel->hasSelection()) {
        if (index.row() >= model_->rowCount())
            index = model_->index(model_->rowCount() - 1, 0);
        else
            index = model_->index(index.row(), 0);
        selectionModel->select(index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Current |
                                          QItemSelectionModel::Rows);
    }
}

void
ChannelConnectionWindow::on_moveUp__clicked()
{
    QItemSelectionModel* selectionModel = connections_->selectionModel();
    if (!selectionModel->hasSelection()) return;
    int row = selectionModel->selection().front().top();
    model_->moveUp(row);
    selectionModel->select(model_->index(row - 1, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void
ChannelConnectionWindow::on_moveDown__clicked()
{
    QItemSelectionModel* selectionModel = connections_->selectionModel();
    if (!selectionModel->hasSelection()) return;
    int row = selectionModel->selection().front().top();
    model_->moveDown(row);
    selectionModel->select(model_->index(row + 1, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
}

void
ChannelConnectionWindow::activeDisplayViewChanged(DisplayView* displayView)
{
    static Logger::ProcLog log("activeDisplayViewChanged", Log());
    LOGINFO << displayView << std::endl;
    unconnected_->setEnabled(false);
    add_->setEnabled(false);
    remove_->setEnabled(false);
    moveUp_->setEnabled(false);
    moveDown_->setEnabled(false);
    Visualizer* v = displayView ? displayView->getVisualizer() : 0;
    model_->setVisualizer(v);
}

void
ChannelConnectionWindow::closeEvent(QCloseEvent* event)
{
    if (getApp()->isQuitting() && model_) {
        model_->saveVideoChannelSettings();
        Super::closeEvent(event);
    }
}

void
ChannelConnectionWindow::keyPressEvent(QKeyEvent* event)
{
    if (event->modifiers() == Qt::ControlModifier) {
        if (event->key() == Qt::Key_Up) {
            on_moveUp__clicked();
            return;
        } else if (event->key() == Qt::Key_Down) {
            on_moveDown__clicked();
            return;
        }
    }
    Super::keyPressEvent(event);
}
