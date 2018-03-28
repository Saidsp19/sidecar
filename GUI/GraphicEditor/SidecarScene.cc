#include "SidecarScene.h"
#include "LinkView.h"
#include "MachineView.h"
#include <QGraphicsSceneMouseEvent>

SidecarScene::SidecarScene(QObject* parent) :
    QGraphicsScene(parent), command(A_none), componentType(ComponentView::CT_algorithm), currentComponent(0)
{
    setSceneRect(-1000, -1000, 2000, 2000);
}

void
SidecarScene::mousePressEvent(QGraphicsSceneMouseEvent* e)
{
    QGraphicsItem* item = itemAt(e->scenePos());

    switch (command) {
    case A_none: break;
    case A_component:
        if (item) {
            MachineView* tmp = qgraphicsitem_cast<MachineView*>(item);
            if (tmp) {
                ComponentView* c = new ComponentView(componentType, tmp);
                addItem(c);
                c->setPos(c->mapFromScene(e->scenePos()));
                currentComponent = c;
                tmp->resize();
                update();
                return;
            }
        } else {
            ComponentView* tmp = new ComponentView(componentType);
            addItem(tmp);
            tmp->setPos(e->scenePos());
            currentComponent = tmp;
            update();
            return;
        }
        break;
    case A_machine:
        if (!item) {
            MachineView* tmp = new MachineView();
            tmp->setZValue(-1); // Machines should be below algorithms
            addItem(tmp);
            tmp->setPos(e->scenePos());
            update();
            return;
        }
        break;
    case A_link:
        if (item) {
            ComponentView* tmp = qgraphicsitem_cast<ComponentView*>(item);
            if (tmp) {
                if (currentComponent && currentComponent != tmp &&
                    currentComponent->parentItem() == tmp->parentItem()) {
                    LinkView* link = new LinkView(currentComponent, tmp);
                    link->setZValue(1); // Links should be above algorithms
                    addItem(link);
                    update();
                }
                currentComponent = tmp;
            }
        }
        return;
    case A_edit:
        if (item) {
            // Display some sort of dialog box (maybe on the left) to edit this component.
            // dialog allows changing of algorithm type, notes, algorithm parameters, filenames, etc.
            /* Pseudocode:
               if(item!=shownItem)
               {
               ComponentView *tmp=qgraphicsitem_cast<ComponentView *>(item);
               if(tmp)
               {
               showDialog(tmp->configDialog());
               shownItem=tmp;
               }
               }
            */
        }
        return;
    case A_delete:
        if (item) {
            if (item == currentComponent) currentComponent = 0;
            delete (item);
            update();
        }
        return;
    default: // remove this once all the commands are accounted for
        break;
    }

    e->ignore();
    return QGraphicsScene::mousePressEvent(e);
}
