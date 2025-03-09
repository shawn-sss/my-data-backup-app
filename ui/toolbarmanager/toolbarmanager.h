#ifndef TOOLBARMANAGER_H
#define TOOLBARMANAGER_H

// Built-in Qt includes
#include <QAction>
#include <QToolBar>
#include <QObject>

// Built-in standard C++ library
#include <array>

// Manages the application's toolbar appearance and actions
class ToolbarManager : public QObject {
    Q_OBJECT

public:
    // Constructor
    explicit ToolbarManager(QObject *parent = nullptr);

    // Initializes the toolbar by applying styles and adding actions
    void initialize(QToolBar *toolBar);

    // Retrieves toolbar actions
    QAction* getActionOpenSettings() const;
    QAction* getActionExit() const;
    QAction* getActionHelp() const;
    QAction* getActionAbout() const;

private:
    // Configures toolbar appearance
    void setupAppearance(QToolBar *toolBar);
    // Adds all actions to the toolbar
    void addActions(QToolBar *toolBar);

    // Stores toolbar actions
    std::array<QAction*, 4> actions;
};

#endif // TOOLBARMANAGER_H
