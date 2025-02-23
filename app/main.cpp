#include <QApplication>
#include "../core/config/_constants.h"
#include "../core/config/configmanager/configmanager.h"
#include "../ui/mainwindow/mainwindow.h"

int main(int argc, char *argv[]) {
    // Initialize application
    QApplication app(argc, argv);

    // Load configuration
    ConfigManager::getInstance();

    // Set application properties
    app.setApplicationName(AppInfo::APP_DISPLAY_TITLE);
    app.setApplicationDisplayName(AppInfo::APP_DISPLAY_TITLE);

    // Initialize main window
    MainWindow mainWindow;
    mainWindow.setWindowTitle(AppInfo::APP_DISPLAY_TITLE);
    mainWindow.setWindowIcon(QIcon(BackupResources::ICON_PATH));
    mainWindow.show();

    // Start event loop
    return app.exec();
}
