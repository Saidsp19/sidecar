#include "QtGui/QApplication"
#include "QtGui/QMessageBox"

int
main(int argc, char** argv)
{
    QApplication app(argc, argv);
    QMessageBox::critical(0, argv[1], argv[2]);
    return 0;
}
