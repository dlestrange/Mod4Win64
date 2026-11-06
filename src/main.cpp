#include "ClassicWindow.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
#ifdef Q_OS_MACOS
    app.setApplicationName("Mod4Mac64");
#else
    app.setApplicationName("Mod4Win64");
#endif
    app.setOrganizationName("Mod4Win64");
    app.setStyle(QStyleFactory::create("Fusion"));
    ClassicPlayer player;
    player.show();
    return app.exec();
}
