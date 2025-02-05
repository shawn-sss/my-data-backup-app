#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "../BackupModule/fileoperations.h"
#include "../Utils/constants.h"
#include "../Utils/utils.h"
#include "../Utils/filewatcher.h"
#include "../BackupModule/stagingmodel.h"
#include "../BackupModule/backupcontroller.h"
#include "../BackupModule/backupservice.h"

#include <QTreeView>
#include <QFileDialog>
#include <QPixmap>
#include <QPainter>
#include <QJsonObject>
#include <QBuffer>
#include <QMessageBox>
#include <QCloseEvent>
#include <QStringList>
#include <QFileSystemModel>
#include <QProgressBar>

// Constructor and Destructor
// --------------------------

// Constructor to initialize the MainWindow
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    destinationModel(new QFileSystemModel(this)),
    sourceModel(new QFileSystemModel(this)),
    fileWatcher(new FileWatcher(this)),
    backupService(new BackupService(UserSettings::DEFAULT_BACKUP_DESTINATION)),
    stagingModel(new StagingModel(this)),
    backupController(new BackupController(backupService, this)) {
    ui->setupUi(this);

    // Configure progress bar
    Utils::UI::setupProgressBar(ui->TransferProgressBar,
                                UIConfig::PROGRESS_BAR_MIN_VALUE,
                                UIConfig::PROGRESS_BAR_MAX_VALUE,
                                UIConfig::PROGRESS_BAR_HEIGHT,
                                UIConfig::PROGRESS_BAR_TEXT_VISIBLE);
    ui->TransferProgressBar->setVisible(false);

    // Ensure the backup directory exists
    if (!FileOperations::createDirectory(UserSettings::DEFAULT_BACKUP_DESTINATION)) {
        QMessageBox::critical(this, UIConfig::INITIALIZATION_ERROR_TITLE, UIConfig::INITIALIZATION_ERROR_MESSAGE);
    }

    // Connect backup signals
    connect(backupController, &BackupController::backupCreated, this, [this]() {
        QMessageBox::information(this, BackupInfo::SUCCESS_BACKUP_CREATED, BackupInfo::BACKUP_COMPLETE_MESSAGE);
        refreshBackupStatus();
    });
    connect(backupController, &BackupController::backupDeleted, this, [this]() {
        QMessageBox::information(this, BackupInfo::SUCCESS_BACKUP_DELETED, BackupInfo::BACKUP_DELETE_SUCCESS);
        refreshBackupStatus();
    });
    connect(backupController, &BackupController::errorOccurred, this, [this](const QString &error) {
        QMessageBox::critical(this, UIConfig::BACKUP_ERROR_TITLE, error);
    });

    // Setup UI components
    setupDestinationView();
    setupSourceTreeView();
    setupBackupStagingTreeView();

    // Initialize backup and file watcher
    refreshBackupStatus();
    startWatchingBackupDirectory(UserSettings::DEFAULT_BACKUP_DESTINATION);
    updateFileWatcher();

    // Connect UI buttons to slots
    connect(ui->AddToBackupButton, &QPushButton::clicked, this, &MainWindow::onAddToBackupClicked);
    connect(ui->ChangeBackupDestinationButton, &QPushButton::clicked, this, &MainWindow::onChangeBackupDestinationClicked);
    connect(ui->RemoveFromBackupButton, &QPushButton::clicked, this, &MainWindow::onRemoveFromBackupClicked);
    connect(ui->CreateBackupButton, &QPushButton::clicked, this, &MainWindow::onCreateBackupClicked);
    connect(ui->DeleteBackupButton, &QPushButton::clicked, this, &MainWindow::onDeleteBackupClicked);
    connect(ui->AboutButton, &QPushButton::clicked, this, &MainWindow::onAboutButtonClicked);
}

// Destructor to clean up resources
MainWindow::~MainWindow() {
    delete ui;
}

// UI Setup
// --------

// Setup the destination view
void MainWindow::setupDestinationView() {
    destinationModel->setFilter(BackupInfo::FILE_SYSTEM_FILTER);
    destinationModel->sort(0, Qt::DescendingOrder);
    ui->BackupDestinationView->setModel(destinationModel);

    QModelIndex rootIndex = destinationModel->setRootPath(UserSettings::DEFAULT_BACKUP_DESTINATION);
    ui->BackupDestinationView->setRootIndex(rootIndex);

    removeAllColumnsFromTreeView(ui->BackupDestinationView);
}

// Setup the source tree view
void MainWindow::setupSourceTreeView() {
    sourceModel->setRootPath(BackupInfo::DEFAULT_ROOT_PATH);
    sourceModel->setFilter(BackupInfo::FILE_SYSTEM_FILTER);
    ui->DriveTreeView->setModel(sourceModel);
    ui->DriveTreeView->setRootIndex(sourceModel->index(BackupInfo::DEFAULT_ROOT_PATH));

    ui->DriveTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    removeAllColumnsFromTreeView(ui->DriveTreeView);
}

// Setup the backup staging tree view
void MainWindow::setupBackupStagingTreeView() {
    ui->BackupStagingTreeView->setModel(stagingModel);
    ui->BackupStagingTreeView->header()->setVisible(true);
    ui->BackupStagingTreeView->header()->setStretchLastSection(true);

    ui->BackupStagingTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);

    removeAllColumnsFromTreeView(ui->BackupStagingTreeView);
}

// Remove all columns from backup staging tree view
void MainWindow::removeAllColumnsFromTreeView(QTreeView *treeView) {
    if (!treeView) return;

    QAbstractItemModel *model = treeView->model();
    if (model) {
        for (int i = UIConfig::TREE_VIEW_START_HIDDEN_COLUMN;
             i < UIConfig::TREE_VIEW_DEFAULT_COLUMN_COUNT; ++i) {
            treeView->setColumnHidden(i, true);
        }
    }
}

// Backup Management
// ------------------

// Add selected items to the backup staging area
void MainWindow::onAddToBackupClicked() {
    QModelIndexList selectedIndexes = ui->DriveTreeView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, UIConfig::NO_BACKUP_ITEMS_SELECTED_TITLE, UIConfig::ERROR_INVALID_SELECTION);
        return;
    }
    Utils::Backup::addSelectedPathsToStaging(ui->DriveTreeView, stagingModel);
}

// Remove selected items from the backup staging area
void MainWindow::onRemoveFromBackupClicked() {
    QModelIndexList selectedIndexes = ui->BackupStagingTreeView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, UIConfig::NO_FILES_SELECTED_TITLE, UIConfig::ERROR_INVALID_SELECTION);
        return;
    }
    Utils::Backup::removeSelectedPathsFromStaging(ui->BackupStagingTreeView, stagingModel);
}

// Change the backup destination directory
void MainWindow::onChangeBackupDestinationClicked() {
    QString selectedDir = QFileDialog::getExistingDirectory(this,
                                                            UIConfig::SELECT_BACKUP_DESTINATION_TITLE,
                                                            BackupInfo::DEFAULT_FILE_DIALOG_ROOT);
    if (selectedDir.isEmpty()) {
        QMessageBox::warning(this, UIConfig::LOCATION_SELECTION_TITLE, UIConfig::ERROR_EMPTY_PATH_SELECTION);
        return;
    }

    if (!FileOperations::createDirectory(selectedDir)) {
        QMessageBox::critical(this, UIConfig::BACKUP_ERROR_TITLE, BackupInfo::ERROR_SET_BACKUP_DESTINATION);
        return;
    }

    QModelIndex rootIndex = destinationModel->setRootPath(selectedDir);
    ui->BackupDestinationView->setRootIndex(rootIndex);

    backupService->setBackupRoot(selectedDir);
    refreshBackupStatus();
    startWatchingBackupDirectory(selectedDir);
    updateFileWatcher();
}

// Create a backup using the selected paths
void MainWindow::onCreateBackupClicked() {
    QStringList pathsToBackup = stagingModel->getStagedPaths();
    if (pathsToBackup.isEmpty()) {
        QMessageBox::warning(this, UIConfig::EMPTY_SELECTION_TITLE, UIConfig::EMPTY_STAGING_WARNING_MESSAGE);
        return;
    }

    ui->TransferProgressBar->setValue(UIConfig::PROGRESS_BAR_MIN_VALUE);
    ui->TransferProgressBar->setVisible(true);

    backupController->createBackup(destinationModel->rootPath(), pathsToBackup, ui->TransferProgressBar);
}

// Delete the selected backup
void MainWindow::onDeleteBackupClicked() {
    QModelIndex selectedIndex = ui->BackupDestinationView->currentIndex();
    if (!selectedIndex.isValid()) {
        QMessageBox::warning(this, UIConfig::BACKUP_ERROR_TITLE, BackupInfo::ERROR_BACKUP_DELETION_FAILED);
        return;
    }

    QString selectedPath = destinationModel->filePath(selectedIndex);
    QDir backupDir(selectedPath);
    QString summaryFilePath = backupDir.filePath(UserSettings::BACKUP_SUMMARY_FILENAME);

    if (!QFile::exists(summaryFilePath)) {
        QMessageBox::warning(this, BackupInfo::INVALID_BACKUP_TITLE, BackupInfo::INVALID_BACKUP_MESSAGE);
        return;
    }

    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        UIConfig::DELETE_BACKUP_WARNING_TITLE,
        QString(UIConfig::DELETE_BACKUP_WARNING_MESSAGE).arg(selectedPath),
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        backupController->deleteBackup(selectedPath);
    }
}

// Display the about dialog
void MainWindow::onAboutButtonClicked() {
    QMessageBox::information(
        this,
        AppConfig::ABOUT_DIALOG_TITLE,
        QString(AppConfig::ABOUT_DIALOG_MESSAGE)
            .arg(AppConfig::APP_VERSION, AppConfig::WINDOW_TITLE)
        );
}

// File and Directory Monitoring
// -----------------------------

// Start watching the backup directory
void MainWindow::startWatchingBackupDirectory(const QString &path) {
    fileWatcher->startWatching(path);
    connect(fileWatcher, &FileWatcher::directoryChanged, this, &MainWindow::onBackupDirectoryChanged);
}

// Update the file watcher to refresh watched paths
void MainWindow::updateFileWatcher() {
    fileWatcher->startWatching(destinationModel->rootPath());
}

// Handle file change events
void MainWindow::onFileChanged(const QString &path) {
    Q_UNUSED(path);
    refreshBackupStatus();
}

// Handle backup directory change events
void MainWindow::onBackupDirectoryChanged() {
    updateFileWatcher();
    refreshBackupStatus();
}

// Status Updates
// ---------------

// Update the backup status label
void MainWindow::updateBackupStatusLabel(bool backupFound) {
    QString color = backupFound
                        ? UIConfig::BACKUP_STATUS_COLOR_FOUND
                        : UIConfig::BACKUP_STATUS_COLOR_NOT_FOUND;

    QPixmap pixmap = Utils::UI::createStatusLightPixmap(color, UIConfig::STATUS_LIGHT_SIZE);

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");

    QString pixmapHtml = QStringLiteral("<img src='data:image/png;base64,%1' style='%2'>")
                             .arg(QString::fromUtf8(ba.toBase64()),
                                  QString(UIConfig::ICON_STYLE_TEMPLATE)
                                      .arg(UIConfig::STATUS_LIGHT_SIZE));

    QString combinedHtml = QStringLiteral(
                               "<div style='display:flex; align-items:center;'>"
                               "<span>%1</span><span style='margin-left:4px;'>%2</span>"
                               "</div>")
                               .arg(UIConfig::LABEL_BACKUP_FOUND, pixmapHtml);

    ui->BackupStatusLabel->setTextFormat(Qt::RichText);
    ui->BackupStatusLabel->setText(combinedHtml);

    ui->LastBackupNameLabel->setVisible(backupFound);
    ui->LastBackupTimestampLabel->setVisible(backupFound);
    ui->LastBackupDurationLabel->setVisible(backupFound);
    ui->LastBackupSizeLabel->setVisible(backupFound);
}

// Update the backup location label
void MainWindow::updateBackupLocationLabel(const QString &location) {
    ui->BackupLocationLabel->setText(UIConfig::LABEL_BACKUP_LOCATION + location);
}

// Update the backup total count label
void MainWindow::updateBackupTotalCountLabel() {
    ui->BackupTotalCountLabel->setText(UIConfig::LABEL_BACKUP_TOTAL_COUNT +
                                       QString::number(backupService->getBackupCount()));
}

// Update the backup total size label
void MainWindow::updateBackupTotalSizeLabel() {
    quint64 totalSize = backupService->getTotalBackupSize();
    QString humanReadableSize = Utils::Formatting::formatSize(totalSize);
    ui->BackupTotalSizeLabel->setText(UIConfig::LABEL_BACKUP_TOTAL_SIZE + humanReadableSize);
}

// Update the backup location status label
void MainWindow::updateBackupLocationStatusLabel(const QString &location) {
    QFileInfo dirInfo(location);
    QString status = dirInfo.exists() ? (dirInfo.isWritable() ? UIConfig::DIRECTORY_STATUS_WRITABLE
                                                              : UIConfig::DIRECTORY_STATUS_READ_ONLY)
                                      : UIConfig::DIRECTORY_STATUS_UNKNOWN;

    ui->BackupLocationStatusLabel->setText(UIConfig::LABEL_BACKUP_LOCATION_ACCESS + status);
}

// Refresh the backup status
void MainWindow::refreshBackupStatus() {
    bool backupFound = backupService->scanForBackupSummary();
    updateBackupStatusLabel(backupFound);
    updateBackupLocationLabel(UserSettings::DEFAULT_BACKUP_DESTINATION);
    updateBackupTotalCountLabel();
    updateBackupTotalSizeLabel();
    updateBackupLocationStatusLabel(UserSettings::DEFAULT_BACKUP_DESTINATION);
    updateLastBackupInfo();
}

// Update information about the last backup
void MainWindow::updateLastBackupInfo() {
    QJsonObject metadata = backupService->getLastBackupMetadata();
    if (metadata.isEmpty()) {
        ui->LastBackupNameLabel->setText(UIConfig::LABEL_LAST_BACKUP_NAME + UIConfig::DEFAULT_VALUE_NOT_AVAILABLE);
        ui->LastBackupTimestampLabel->setText(UIConfig::LABEL_LAST_BACKUP_TIMESTAMP + UIConfig::DEFAULT_VALUE_NOT_AVAILABLE);
        ui->LastBackupDurationLabel->setText(UIConfig::LABEL_LAST_BACKUP_DURATION + UIConfig::DEFAULT_VALUE_NOT_AVAILABLE);
        ui->LastBackupSizeLabel->setText(UIConfig::LABEL_LAST_BACKUP_SIZE + UIConfig::DEFAULT_VALUE_NOT_AVAILABLE);
        return;
    }

    ui->LastBackupNameLabel->setText(UIConfig::LABEL_LAST_BACKUP_NAME +
                                     metadata.value("backup_name").toString(UIConfig::DEFAULT_VALUE_NOT_AVAILABLE));

    QDateTime backupTimestamp = QDateTime::fromString(metadata.value("backup_timestamp").toString(), Qt::ISODate);
    ui->LastBackupTimestampLabel->setText(UIConfig::LABEL_LAST_BACKUP_TIMESTAMP +
                                          Utils::Formatting::formatTimestamp(backupTimestamp, BackupInfo::BACKUP_TIMESTAMP_DISPLAY_FORMAT));

    ui->LastBackupDurationLabel->setText(UIConfig::LABEL_LAST_BACKUP_DURATION +
                                         Utils::Formatting::formatDuration(metadata.value("backup_duration").toInt(0)));

    ui->LastBackupSizeLabel->setText(UIConfig::LABEL_LAST_BACKUP_SIZE +
                                     metadata.value("total_size_readable").toString(UIConfig::DEFAULT_VALUE_NOT_AVAILABLE));
}

// Event Handlers
// --------------

// Handle the close event for the MainWindow
void MainWindow::closeEvent(QCloseEvent *event) {
    if (backupController && backupController->isBackupInProgress()) {
        QMessageBox::warning(this,
                             UIConfig::MESSAGE_OPERATION_IN_PROGRESS,
                             UIConfig::MESSAGE_OPERATION_WARNING);
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}
