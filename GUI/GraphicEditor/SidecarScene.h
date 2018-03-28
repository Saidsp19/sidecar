#ifndef SIDECAR_SCENE_H
#define SIDECAR_SCENE_H

#include "ComponentView.h"

#include <QGraphicsScene>

class SidecarScene : public QGraphicsScene {
    Q_OBJECT

public:
    SidecarScene(QObject* parent = 0);

    enum Action { A_none = 0, A_component, A_link, A_machine, A_edit, A_delete, A_copy, A_paste, A_group, A_ungroup };

    void setCommand(Action a) { command = a; }
    void setCommand(ComponentView::ComponentType c)
    {
        command = A_component;
        componentType = c;
    }

    void mousePressEvent(QGraphicsSceneMouseEvent*);

private:
    Action command;
    ComponentView::ComponentType componentType;
    ComponentView* currentComponent;
};

/** \file
 */

#endif
