#include "PlayerWindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Mod4Win64");
    app.setOrganizationName("Mod4Win64");
    app.setStyle(QStyleFactory::create("Fusion"));

    PlayerWindow window;
    window.show();
    return app.exec();
}
