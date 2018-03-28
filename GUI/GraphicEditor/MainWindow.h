#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "ui_MainWindow.h"

class SidecarScene;

class MainWindow : public QMainWindow, private Ui::MainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = 0);

private:
    SidecarScene* scene;

private slots:
    void handle_button_press(int id);
};

/** \file
 */

#endif
