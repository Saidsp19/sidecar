#include "QtCore/QSettings"
#include "QtCore/QTimer"
#include "QtGui/QCloseEvent"

#include "AlgorithmItem.h"
#include "App.h"
#include "ChannelItem.h"
#include "ChannelListItem.h"
#include "ConfigurationItem.h"
#include "MainWindow.h"
#include "MessageType.h"
#include "RunnerItem.h"
#include "StreamItem.h"
#include "TreeModel.h"
#include "ui_MainWindow.h"

using namespace SideCar::GUI::ConfigEditor;

MainWindow::MainWindow() : MainWindowBase(), gui_(new Ui::MainWindow), model_(new TreeModel(this))
{
    gui_->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);
    setObjectName("MainWindow");
    gui_->view_->setModel(model_);

    MessageType* videoType = MessageType::Make("Video");
    MessageType* binaryVideoType = MessageType::Make("BinaryVideo");

    ConfigurationItem* config = new ConfigurationItem(0, "Config 1");

    RunnerItem* runner = new RunnerItem(0, "Runner 1", "Host", "Addr");
    config->insertChild(0, runner);

    StreamItem* stream = new StreamItem(runner, "Stream 1");
    runner->insertChild(0, stream);
    TaskItem* task = new TaskItem(stream, "Subscriber 1");
    stream->insertChild(0, task);
    task->getOutputChannels()->insertChild(0, new ChannelItem(task->getOutputChannels(), "A", "", videoType));

    task = new AlgorithmItem(stream, "Algorithm 1", "One");
    stream->insertChild(1, task);
    task->getInputChannels()->insertChild(0, new ChannelItem(task->getInputChannels(), "A", "In", videoType));
    task->getOutputChannels()->insertChild(0, new ChannelItem(task->getOutputChannels(), "B", "Out", binaryVideoType));

    task = new AlgorithmItem(stream, "Algorithm 2", "Two");
    stream->insertChild(2, task);

    task->getInputChannels()->insertChild(0, new ChannelItem(task->getInputChannels(), "B", "In", binaryVideoType));
    task->getOutputChannels()->insertChild(0, new ChannelItem(task->getOutputChannels(), "C", "Out", binaryVideoType));

    task = new TaskItem(stream, "Publisher 1");
    stream->insertChild(3, task);
    task->getInputChannels()->insertChild(0, new ChannelItem(task->getInputChannels(), "C", "", binaryVideoType));

    QList<TreeItem*> items;
    items.append(config);
    model_->insertItems(0, items);

    gui_->view_->expandAll();
    gui_->view_->resizeColumnToContents(0);
}
