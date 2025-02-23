#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "../../core/config/_constants.h"
#include "../../core/config/resources_settings.h"
#include "../../core/backup_module/controller/backupcontroller.h"
#include "../../core/backup_module/service/backupservice.h"
#include "../../core/backup_module/models/stagingmodel.h"
#include "../../core/utils/file_utils/fileoperations.h"
#include "../../core/utils/common_utils/utils.h"
#include "../../core/utils/file_utils/filewatcher.h"
#include "../../ui/settingsdialog/settingsdialog.h"

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
#include <QMenu>
#include <QLayout>
#include <QToolBar>
#include <QWidgetAction>
#include <QApplication>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QWidget>
#include <QtSvg/QSvgRenderer>
#include <QPainter>

// Constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    destinationModel(new QFileSystemModel(this)),
    sourceModel(new QFileSystemModel(this)),
    fileWatcher(new FileWatcher(this)),
    backupService(new BackupService(UserConfig::DEFAULT_BACKUP_DIRECTORY)),
    stagingModel(new StagingModel(this)),
    backupController(new BackupController(backupService, this)) {

    ui->setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setStatusBar(nullptr);
    titleBar = Utils::setupCustomTitleBar(this, TitleBarMode::MainWindow);

    if (ui->mainToolBar) {
        ui->mainToolBar->setFloatable(false);
        ui->mainToolBar->setMovable(false);
        ui->mainToolBar->setVisible(true);
        ui->mainToolBar->setIconSize(QSize(24, 24));
        ui->mainToolBar->setStyleSheet("background-color: transparent; border: none;");
    }

    initializeUI();
    initializeBackupSystem();
    setupConnections();

    ui->AddToBackupButton->setCursor(Qt::PointingHandCursor);
    ui->RemoveFromBackupButton->setCursor(Qt::PointingHandCursor);
    ui->CreateBackupButton->setCursor(Qt::PointingHandCursor);
    ui->ChangeBackupDestinationButton->setCursor(Qt::PointingHandCursor);
    ui->DeleteBackupButton->setCursor(Qt::PointingHandCursor);
}

// Destructor
MainWindow::~MainWindow() {
    delete ui;
}

// Initializes UI components
void MainWindow::initializeUI() {
    setupCustomTitleBar();
    setupToolBar();
    Utils::UI::setupProgressBar(ui->TransferProgressBar,
                                ProgressConfig::PROGRESS_BAR_MIN_VALUE,
                                ProgressConfig::PROGRESS_BAR_MAX_VALUE,
                                ProgressConfig::PROGRESS_BAR_HEIGHT,
                                ProgressConfig::PROGRESS_BAR_TEXT_VISIBLE);
    ui->TransferProgressBar->setVisible(false);
}

// Sets up the custom title bar
void MainWindow::setupCustomTitleBar() {
    titleBar = Utils::setupCustomTitleBar(this, TitleBarMode::MainWindow);
    if (!titleBar) return;

    QWidget *container = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(container);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(titleBar, 0, Qt::AlignTop);

    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);

    if (ui->mainToolBar) {
        ui->mainToolBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        contentLayout->addWidget(ui->mainToolBar, 0, Qt::AlignLeft);
    }

    if (centralWidget()) {
        QWidget *contentContainer = centralWidget();
        contentContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        contentLayout->addWidget(contentContainer, 1);
    }

    mainLayout->addLayout(contentLayout, 1);
    container->setLayout(mainLayout);
    setCentralWidget(container);
}

// Initializes the backup system
void MainWindow::initializeBackupSystem() {
    if (!FileOperations::createDirectory(UserConfig::DEFAULT_BACKUP_DIRECTORY)) {
        QMessageBox::critical(this, ErrorMessages::BACKUP_INITIALIZATION_FAILED_TITLE,
                              ErrorMessages::ERROR_CREATING_DEFAULT_BACKUP_DIRECTORY);
    }

    setupDestinationView();
    setupSourceTreeView();
    setupBackupStagingTreeView();
    refreshBackupStatus();

    if (!fileWatcher->watchedDirectories().contains(UserConfig::DEFAULT_BACKUP_DIRECTORY)) {
        startWatchingBackupDirectory(UserConfig::DEFAULT_BACKUP_DIRECTORY);
    }
}


// Sets up signal connections
void MainWindow::setupConnections() {
    connectBackupSignals();
    connect(ui->AddToBackupButton, &QPushButton::clicked, this, &MainWindow::onAddToBackupClicked);
    connect(ui->ChangeBackupDestinationButton, &QPushButton::clicked, this, &MainWindow::onChangeBackupDestinationClicked);
    connect(ui->RemoveFromBackupButton, &QPushButton::clicked, this, &MainWindow::onRemoveFromBackupClicked);
    connect(ui->CreateBackupButton, &QPushButton::clicked, this, &MainWindow::onCreateBackupClicked);
    connect(ui->DeleteBackupButton, &QPushButton::clicked, this, &MainWindow::onDeleteBackupClicked);
    connect(ui->actionOpenSettings, &QAction::triggered, this, &MainWindow::openSettings);
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::exitApplication);
    connect(ui->actionHelp, &QAction::triggered, this, &MainWindow::showHelpDialog);
    connect(ui->actionAbout, &QAction::triggered, this, &MainWindow::onAboutButtonClicked);
}

// Sets up toolbar
void MainWindow::setupToolBar() {
    if (!ui->mainToolBar) return;

    setupToolbarActions();

    ui->mainToolBar->setFloatable(false);
    ui->mainToolBar->setMovable(false);
    ui->mainToolBar->setVisible(true);
    ui->mainToolBar->setIconSize(QSize(24, 24));
    ui->mainToolBar->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    ui->mainToolBar->setStyleSheet("background-color: transparent; border: none; padding: 5px;");

    ui->mainToolBar->clear();

    ui->mainToolBar->addAction(ui->actionOpenSettings);
    ui->mainToolBar->addAction(ui->actionHelp);
    ui->mainToolBar->addAction(ui->actionAbout);

    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->mainToolBar->addWidget(spacer);

    ui->mainToolBar->addAction(ui->actionExit);

    for (QAction *&action : ui->mainToolBar->actions()) {
        QWidget *widget = ui->mainToolBar->widgetForAction(action);
        if (widget) widget->setCursor(Qt::PointingHandCursor);
    }
}

// Sets up toolbar actions
void MainWindow::setupToolbarActions() {
    if (!ui->actionOpenSettings) ui->actionOpenSettings = new QAction(this);
    if (!ui->actionExit) ui->actionExit = new QAction(this);
    if (!ui->actionHelp) ui->actionHelp = new QAction(this);
    if (!ui->actionAbout) ui->actionAbout = new QAction(this);

    ui->actionOpenSettings->setIcon(QIcon(BackupResources::SETTINGS_ICON_PATH));
    ui->actionOpenSettings->setText(UIConfig::MENU_SETTINGS_LABEL);

    ui->actionExit->setIcon(QIcon(BackupResources::EXIT_ICON_PATH));
    ui->actionExit->setText(UIConfig::MENU_EXIT_LABEL);

    ui->actionHelp->setIcon(QIcon(BackupResources::HELP_ICON_PATH));
    ui->actionHelp->setText(UIConfig::MENU_HELP_LABEL);

    ui->actionAbout->setIcon(QIcon(BackupResources::ABOUT_ICON_PATH));
    ui->actionAbout->setText(UIConfig::MENU_ABOUT_LABEL);
}

// Adds a spacer to the toolbar
void MainWindow::addToolbarSpacer() {
    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->mainToolBar->addWidget(spacer);
}

// Connects toolbar actions to their respective slots
void MainWindow::connectToolbarActions(QAction* helpAction, QAction* aboutAction) {
    connect(ui->actionExit, &QAction::triggered, this, &MainWindow::exitApplication);
    connect(helpAction, &QAction::triggered, this, &MainWindow::showHelpDialog);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::onAboutButtonClicked);
}

// Handles mouse events for dragging
void MainWindow::handleMouseEvent(QMouseEvent *event, bool isPress) {
    if (event->button() == Qt::LeftButton) {
        dragging = isPress;
        if (isPress) {
            lastMousePosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        }
        event->accept();
    }
}

// Handles mouse press events
void MainWindow::mousePressEvent(QMouseEvent *event) {
    handleMouseEvent(event, true);
}

// Handles mouse move events
void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (dragging && event->buttons() & Qt::LeftButton) {
        move(event->globalPosition().toPoint() - lastMousePosition);
        event->accept();
    }
}

// Handles mouse release events
void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    handleMouseEvent(event, false);
}

// Refreshes the backup status
void MainWindow::refreshBackupStatus() {
    BackupStatus status = backupService->scanForBackupStatus();

    switch (status) {
    case BackupStatus::Valid:
        updateBackupStatusLabel(Colors::BACKUP_STATUS_COLOR_FOUND);
        break;
    case BackupStatus::Broken:
        updateBackupStatusLabel(Colors::BACKUP_STATUS_COLOR_WARNING);
        break;
    case BackupStatus::None:
    default:
        updateBackupStatusLabel(Colors::BACKUP_STATUS_COLOR_NOT_FOUND);
        break;
    }

    updateBackupLocationLabel(backupService->getBackupRoot());
    updateBackupTotalCountLabel();
    updateBackupTotalSizeLabel();
    updateBackupLocationStatusLabel(backupService->getBackupRoot());
    updateLastBackupInfo();
}

// Changes the backup destination
void MainWindow::onChangeBackupDestinationClicked() {
    QString selectedDir = QFileDialog::getExistingDirectory(this,
                                                            InfoMessages::SELECT_BACKUP_DESTINATION_TITLE,
                                                            BackupInfo::DEFAULT_FILE_DIALOG_ROOT);
    if (selectedDir.isEmpty()) {
        QMessageBox::warning(this, ErrorMessages::BACKUP_LOCATION_REQUIRED_TITLE, ErrorMessages::ERROR_NO_BACKUP_LOCATION_PATH_SELECTED);
        return;
    }

    if (!FileOperations::createDirectory(selectedDir)) {
        QMessageBox::critical(this, ErrorMessages::BACKUP_DIRECTORY_ERROR_TITLE, ErrorMessages::ERROR_CREATING_BACKUP_DIRECTORY);
        return;
    }

    backupService->setBackupRoot(selectedDir);
    destinationModel->setRootPath(selectedDir);
    ui->BackupDestinationView->setModel(destinationModel);
    ui->BackupDestinationView->setRootIndex(destinationModel->index(selectedDir));

    refreshBackupStatus();
    startWatchingBackupDirectory(selectedDir);
    updateFileWatcher();
}

// Adds selected paths to backup staging
void MainWindow::onAddToBackupClicked() {
    QModelIndexList selectedIndexes = ui->DriveTreeView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, ErrorMessages::BACKUP_SELECTION_REQUIRED_TITLE, ErrorMessages::ERROR_NO_ITEMS_SELECTED_FOR_BACKUP);
        return;
    }
    Utils::Backup::addSelectedPathsToStaging(ui->DriveTreeView, stagingModel);
}

// Removes selected paths from backup staging
void MainWindow::onRemoveFromBackupClicked() {
    QModelIndexList selectedIndexes = ui->BackupStagingTreeView->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) {
        QMessageBox::warning(this, ErrorMessages::REMOVE_SELECTION_REQUIRED_TITLE, ErrorMessages::ERROR_NO_ITEMS_SELECTED_FOR_REMOVAL);
        return;
    }
    Utils::Backup::removeSelectedPathsFromStaging(ui->BackupStagingTreeView, stagingModel);
}

// Initiates backup creation
void MainWindow::onCreateBackupClicked() {
    QStringList pathsToBackup = stagingModel->getStagedPaths();
    if (pathsToBackup.isEmpty()) {
        QMessageBox::warning(this, ErrorMessages::NO_ITEMS_STAGED_FOR_BACKUP_TITLE, ErrorMessages::ERROR_NO_ITEMS_STAGED_FOR_BACKUP);
        return;
    }

    QString backupRoot = destinationModel->rootPath();
    QString errorMessage;

    if (!FileOperations::createBackupInfrastructure(backupRoot, errorMessage)) {
        QMessageBox::critical(this, ErrorMessages::ERROR_BACKUP_ALREADY_IN_PROGRESS, errorMessage);
        return;
    }

    ui->TransferProgressBar->setValue(ProgressConfig::PROGRESS_BAR_MIN_VALUE);
    ui->TransferProgressBar->setVisible(true);

    backupController->createBackup(backupRoot, pathsToBackup, ui->TransferProgressBar);
}

// Deletes a selected backup
void MainWindow::onDeleteBackupClicked() {
    QModelIndex selectedIndex = ui->BackupDestinationView->currentIndex();
    if (!selectedIndex.isValid()) {
        QMessageBox::warning(this, ErrorMessages::BACKUP_DELETION_ERROR_TITLE, ErrorMessages::ERROR_BACKUP_DELETE_FAILED);
        return;
    }

    QString selectedPath = destinationModel->filePath(selectedIndex);

    if (QMessageBox::question(this, WarningMessages::WARNING_CONFIRM_BACKUP_DELETION,
                              QString(WarningMessages::MESSAGE_CONFIRM_BACKUP_DELETION).arg(selectedPath),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes) {
        backupController->deleteBackup(selectedPath);
    }
}

// Starts watching the backup directory
void MainWindow::startWatchingBackupDirectory(const QString &path) {
    fileWatcher->startWatching(path);
    connect(fileWatcher, &FileWatcher::directoryChanged, this, &MainWindow::onBackupDirectoryChanged);
}

// Updates the file watcher
void MainWindow::updateFileWatcher() {
    QString watchPath = destinationModel->rootPath();

    if (!fileWatcher->watchedDirectories().contains(watchPath)) {
        fileWatcher->startWatching(watchPath);
    }
}

// Handles file changes
void MainWindow::onFileChanged(const QString &path) {
    Q_UNUSED(path);
    refreshBackupStatus();
}

// Handles backup directory changes
void MainWindow::onBackupDirectoryChanged() {
    updateFileWatcher();
    refreshBackupStatus();
}

// Updates the backup status label
void MainWindow::updateBackupStatusLabel(const QString &statusColor) {
    QPixmap pixmap = Utils::UI::createStatusLightPixmap(statusColor, ProgressConfig::STATUS_LIGHT_SIZE);
    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    pixmap.save(&buffer, "PNG");
    QString pixmapHtml = QString(Styling::STATUS_LIGHT_ICON_TEMPLATE)
                             .arg(QString::fromUtf8(ba.toBase64()));
    QString combinedHtml = QString(Styling::STATUS_LABEL_HTML_TEMPLATE)
                               .arg(UIConfig::LABEL_BACKUP_FOUND, pixmapHtml);
    ui->BackupStatusLabel->setTextFormat(Qt::RichText);
    ui->BackupStatusLabel->setText(combinedHtml);

    bool backupExists = (statusColor == Colors::BACKUP_STATUS_COLOR_FOUND);
    ui->LastBackupNameLabel->setVisible(backupExists);
    ui->LastBackupTimestampLabel->setVisible(backupExists);
    ui->LastBackupDurationLabel->setVisible(backupExists);
    ui->LastBackupSizeLabel->setVisible(backupExists);
}

// Updates the backup location label
void MainWindow::updateBackupLocationLabel(const QString &location) {
    ui->BackupLocationLabel->setText(UIConfig::LABEL_BACKUP_LOCATION + location);
}

// Updates the backup total count label
void MainWindow::updateBackupTotalCountLabel() {
    ui->BackupTotalCountLabel->setText(UIConfig::LABEL_BACKUP_TOTAL_COUNT +
                                       QString::number(backupService->getBackupCount()));
}

// Updates the backup total size label
void MainWindow::updateBackupTotalSizeLabel() {
    quint64 totalSize = backupService->getTotalBackupSize();
    QString humanReadableSize = Utils::Formatting::formatSize(totalSize);
    ui->BackupTotalSizeLabel->setText(UIConfig::LABEL_BACKUP_TOTAL_SIZE + humanReadableSize);
}

// Updates the backup location status label
void MainWindow::updateBackupLocationStatusLabel(const QString &location) {
    QFileInfo dirInfo(location);
    QString status = dirInfo.exists() ? (dirInfo.isWritable() ? UIConfig::DIRECTORY_STATUS_WRITABLE
                                                              : UIConfig::DIRECTORY_STATUS_READ_ONLY)
                                      : UIConfig::DIRECTORY_STATUS_UNKNOWN;
    ui->BackupLocationStatusLabel->setText(UIConfig::LABEL_BACKUP_LOCATION_ACCESS + status);
}

// Updates the last backup info
void MainWindow::updateLastBackupInfo() {
    QJsonObject metadata = backupService->getLastBackupMetadata();
    if (metadata.isEmpty()) {
        QString notAvailable = Utilities::DEFAULT_VALUE_NOT_AVAILABLE;
        ui->LastBackupNameLabel->setText(UIConfig::LABEL_LAST_BACKUP_NAME + notAvailable);
        ui->LastBackupTimestampLabel->setText(UIConfig::LABEL_LAST_BACKUP_TIMESTAMP + notAvailable);
        ui->LastBackupDurationLabel->setText(UIConfig::LABEL_LAST_BACKUP_DURATION + notAvailable);
        ui->LastBackupSizeLabel->setText(UIConfig::LABEL_LAST_BACKUP_SIZE + notAvailable);
        return;
    }

    QString backupName = metadata.value(BackupMetadataKeys::NAME).toString(Utilities::DEFAULT_VALUE_NOT_AVAILABLE);
    QString timestampStr = metadata.value(BackupMetadataKeys::TIMESTAMP).toString();
    int durationMs = metadata.value(BackupMetadataKeys::DURATION).toInt(0);
    QString totalSize = metadata.value(BackupMetadataKeys::SIZE_READABLE).toString(Utilities::DEFAULT_VALUE_NOT_AVAILABLE);

    QDateTime backupTimestamp = QDateTime::fromString(timestampStr, Qt::ISODate);
    QString formattedTimestamp = Utils::Formatting::formatTimestamp(backupTimestamp, BackupInfo::BACKUP_TIMESTAMP_DISPLAY_FORMAT);
    QString formattedDuration = Utils::Formatting::formatDuration(durationMs);

    ui->LastBackupNameLabel->setText(UIConfig::LABEL_LAST_BACKUP_NAME + backupName);
    ui->LastBackupTimestampLabel->setText(UIConfig::LABEL_LAST_BACKUP_TIMESTAMP + formattedTimestamp);
    ui->LastBackupDurationLabel->setText(UIConfig::LABEL_LAST_BACKUP_DURATION + formattedDuration);
    ui->LastBackupSizeLabel->setText(UIConfig::LABEL_LAST_BACKUP_SIZE + totalSize);
}

// Handles the close event
void MainWindow::closeEvent(QCloseEvent *event) {
    if (backupController->isBackupInProgress()) {
        QMessageBox::warning(this, ErrorMessages::ERROR_OPERATION_IN_PROGRESS,
                             ErrorMessages::WARNING_OPERATION_STILL_RUNNING);
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}

// Sets up the destination view
void MainWindow::setupDestinationView() {
    destinationModel->setFilter(BackupInfo::FILE_SYSTEM_FILTER);
    destinationModel->sort(0, Qt::DescendingOrder);
    ui->BackupDestinationView->setModel(destinationModel);
    ui->BackupDestinationView->setRootIndex(destinationModel->setRootPath(UserConfig::DEFAULT_BACKUP_DIRECTORY));
    removeAllColumnsFromTreeView(ui->BackupDestinationView);
}

// Sets up the backup staging tree view
void MainWindow::setupBackupStagingTreeView() {
    ui->BackupStagingTreeView->setModel(stagingModel);
    ui->BackupStagingTreeView->header()->setVisible(true);
    ui->BackupStagingTreeView->header()->setStretchLastSection(true);
    ui->BackupStagingTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    removeAllColumnsFromTreeView(ui->BackupStagingTreeView);
}

// Opens the settings dialog
void MainWindow::openSettings() {
    SettingsDialog settingsDialog(this);
    settingsDialog.exec();
}

// Exits the application
void MainWindow::exitApplication() {
    QApplication::quit();
}

// Displays the help dialog
void MainWindow::showHelpDialog() {
    QMessageBox::information(this, HelpInfo::HELP_WINDOW_TITLE, HelpInfo::HELP_WINDOW_MESSAGE);
}

// Displays the about dialog
void MainWindow::onAboutButtonClicked() {
    QMessageBox::information(this, AboutInfo::ABOUT_WINDOW_TITLE,
                             QString(AboutInfo::ABOUT_WINDOW_MESSAGE).arg(AppInfo::APP_VERSION, AppInfo::APP_DISPLAY_TITLE));
}

// Connects backup signals
void MainWindow::connectBackupSignals() {
    connect(backupController, &BackupController::backupCreated, this, &MainWindow::refreshBackupStatus);
    connect(backupController, &BackupController::backupDeleted, this, &MainWindow::refreshBackupStatus);
    connect(backupController, &BackupController::errorOccurred, this, [this](const QString &error) {
        QMessageBox::critical(this, ErrorMessages::BACKUP_DELETION_ERROR_TITLE, error);
    });
}

// Sets up the source tree view
void MainWindow::setupSourceTreeView() {
    sourceModel->setRootPath(BackupInfo::DEFAULT_ROOT_PATH);
    sourceModel->setFilter(BackupInfo::FILE_SYSTEM_FILTER);
    ui->DriveTreeView->setModel(sourceModel);
    ui->DriveTreeView->setRootIndex(sourceModel->index(BackupInfo::DEFAULT_ROOT_PATH));
    ui->DriveTreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    removeAllColumnsFromTreeView(ui->DriveTreeView);
}

// Removes unnecessary columns from QTreeView
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
