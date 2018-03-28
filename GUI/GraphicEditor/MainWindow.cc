#include "MainWindow.h"
#include "ComponentView.h"
#include "LinkView.h"
#include "SidecarScene.h"

#include <QButtonGroup>

const int ComponentOffset = 100;

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), scene(new SidecarScene())
{
    setupUi(this);

    graphicsView->setScene(scene);
    graphicsView->setDragMode(QGraphicsView::RubberBandDrag);
    graphicsView->centerOn(0, 0);

    QButtonGroup* exclusive = new QButtonGroup(this);
    exclusive->setExclusive(true);
    exclusive->addButton(toolAlgorithm, ComponentOffset + ComponentView::CT_algorithm);
    exclusive->addButton(toolUDPReader, ComponentOffset + ComponentView::CT_udpreader);
    exclusive->addButton(toolPublisher, ComponentOffset + ComponentView::CT_publisher);
    exclusive->addButton(toolSubscriber, ComponentOffset + ComponentView::CT_subscriber);
    exclusive->addButton(toolFileReader, ComponentOffset + ComponentView::CT_filereader);
    exclusive->addButton(toolFileWriter, ComponentOffset + ComponentView::CT_filewriter);

    exclusive->addButton(pushLink, SidecarScene::A_link);
    exclusive->addButton(pushMachine, SidecarScene::A_machine);

    exclusive->addButton(toolEdit, SidecarScene::A_edit);
    exclusive->addButton(toolDelete, SidecarScene::A_delete);
    exclusive->addButton(toolCopy, SidecarScene::A_copy);
    exclusive->addButton(toolPaste, SidecarScene::A_paste);
    exclusive->addButton(toolGroup, SidecarScene::A_group);
    exclusive->addButton(toolUngroup, SidecarScene::A_ungroup);

    QObject::connect(exclusive, SIGNAL(buttonClicked(int)), this, SLOT(handle_button_press(int)));
}

void
MainWindow::handle_button_press(int id)
{
    if (id >= ComponentOffset) {
        scene->setCommand((ComponentView::ComponentType)(id - ComponentOffset));
    } else {
        scene->setCommand((SidecarScene::Action)id);
    }
}
