#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QProgressBar>
#include <QStringList>

// Forward declarations for custom classes
class QTreeView;
class BackupService;
class StagingModel;
class FileWatcher;
class BackupController;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// Main application window class
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    // Constructor and destructor
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    // Overridden event handlers
    void closeEvent(QCloseEvent *event) override;

private:
    // Initialization methods
    void setupDestinationView();
    void setupSourceTreeView();
    void setupBackupStagingTreeView();

    // Backup management methods
    void updateLastBackupInfo();
    void refreshBackupStatus();

    // File and directory monitoring methods
    void startWatchingBackupDirectory(const QString &path);
    void updateFileWatcher();

    // UI update methods
    void updateBackupStatusLabel(bool backupFound);
    void updateBackupLocationLabel(const QString &location);
    void updateBackupTotalCountLabel();
    void updateBackupTotalSizeLabel();
    void updateBackupLocationStatusLabel(const QString &location);
    void removeAllColumnsFromTreeView(QTreeView *treeView);

private slots:
    // UI interaction slots
    void onAddToBackupClicked();
    void onChangeBackupDestinationClicked();
    void onRemoveFromBackupClicked();
    void onCreateBackupClicked();
    void onDeleteBackupClicked();
    void onAboutButtonClicked();

    // File and directory watcher signal slots
    void onBackupDirectoryChanged();
    void onFileChanged(const QString &path);

private:
    // UI components and models
    Ui::MainWindow *ui;
    QFileSystemModel *destinationModel;
    QFileSystemModel *sourceModel;
    QProgressBar *progressBar;

    // Backup-related objects
    FileWatcher *fileWatcher;
    BackupService *backupService;
    StagingModel *stagingModel;
    BackupController *backupController;
};

#endif // MAINWINDOW_H
