// Project includes same directory
#include "backupservice.h"

// Project includes different directory
#include "../../../config/_constants.h"
#include "../../utils/file_utils/fileoperations.h"
#include "../../../core/utils/common_utils/utils.h"
#include "../../../core/utils/file_utils/jsonmanager.h"

// Built-in Qt includes
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>

// Constructor
BackupService::BackupService(const QString &backupRoot)
    : backupRootPath(backupRoot) {}

// Sets backup root path
void BackupService::setBackupRoot(const QString &path) {
    backupRootPath = path;
}

// Retrieves backup root path
QString BackupService::getBackupRoot() const {
    return backupRootPath;
}

// Initializes backup root directory if needed
void BackupService::initializeBackupRootIfNeeded() {
    const QString configFilePath = QDir(backupRootPath).filePath(QStringLiteral("%1/%2").arg(AppConfig::BACKUP_SETUP_FOLDER, AppConfig::BACKUP_SETUP_INFO_FILE));

    if (QFile::exists(configFilePath)) {
        return;
    }

    QString errorMessage;
    if (!FileOperations::createBackupInfrastructure(backupRootPath, errorMessage)) {
        return;
    }

    QJsonObject backupData;
    backupData["location"] = backupRootPath;
    backupData["timestamp"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);

    QJsonObject backupConfig;
    backupConfig["app_name"] = AppInfo::APP_DISPLAY_TITLE;
    backupConfig["app_author"] = AppInfo::AUTHOR_NAME;
    backupConfig["app_version"] = AppInfo::APP_VERSION;
    backupConfig["backup"] = backupData;

    JsonManager::saveJsonFile(configFilePath, backupConfig);
}

// Calculates total backup size
qint64 BackupService::calculateTotalBackupSize(const QStringList &items) const {
    qint64 totalSize = 0;
    for (const QString &item : items) {
        QFileInfo fileInfo(item);
        totalSize += fileInfo.isDir() ? FileOperations::calculateDirectorySize(QDir(item)) : fileInfo.size();
    }
    return totalSize;
}

// Scans for backup status
BackupStatus BackupService::scanForBackupStatus() const {
    const QDir configDir(QDir(backupRootPath).filePath(AppConfig::BACKUP_SETUP_FOLDER));
    if (!configDir.exists()) return BackupStatus::None;
    bool validLogs = QDir(configDir.filePath(AppConfig::BACKUP_LOGS_FOLDER)).exists();
    bool validConfig = QFile::exists(configDir.filePath(AppConfig::BACKUP_SETUP_INFO_FILE));
    return (validLogs && validConfig) ? BackupStatus::Valid : BackupStatus::Broken;
}

// Retrieves metadata for the last backup
QJsonObject BackupService::getLastBackupMetadata() const {
    const QDir logsDir(QDir(backupRootPath).filePath(QStringLiteral("%1/%2").arg(AppConfig::BACKUP_SETUP_FOLDER, AppConfig::BACKUP_LOGS_FOLDER)));
    QFileInfoList logFiles = logsDir.entryInfoList({"*_" + AppConfig::BACKUP_LOGS_FILE}, QDir::Files, QDir::Time);

    if (logFiles.isEmpty()) {
        return QJsonObject();
    }

    QJsonObject metadata;
    if (JsonManager::loadJsonFile(logFiles.first().absoluteFilePath(), metadata)) {
        return metadata;
    }

    return QJsonObject();  // Return empty if loading fails
}

// Creates a backup summary log
void BackupService::createBackupSummary(const QString &backupFolderPath, const QStringList &selectedItems, qint64 backupDuration) {
    const QString logsFolderPath = QDir(backupRootPath).filePath(QStringLiteral("%1/%2").arg(AppConfig::BACKUP_SETUP_FOLDER, AppConfig::BACKUP_LOGS_FOLDER));
    if (!QDir(logsFolderPath).exists()) {
        QDir().mkpath(logsFolderPath);
    }
    QString logFileName = QFileInfo(backupFolderPath).fileName() + "_" + AppConfig::BACKUP_LOGS_FILE;
    JsonManager::saveJsonFile(QDir(logsFolderPath).filePath(logFileName), createBackupMetadata(backupFolderPath, selectedItems, backupDuration));
}

// Counts total backups
int BackupService::getBackupCount() const {
    const QString logsFolderPath = QDir(backupRootPath).filePath(QStringLiteral("%1/%2").arg(AppConfig::BACKUP_SETUP_FOLDER, AppConfig::BACKUP_LOGS_FOLDER));
    return QDir(logsFolderPath).entryList({"*_" + AppConfig::BACKUP_LOGS_FILE}, QDir::Files).size();
}

// Calculates total backup storage size
quint64 BackupService::getTotalBackupSize() const {
    quint64 totalSize = 0;
    const QDir logsDir(QDir(backupRootPath).filePath(QStringLiteral("%1/%2")
                                                         .arg(AppConfig::BACKUP_SETUP_FOLDER, AppConfig::BACKUP_LOGS_FOLDER)));
    const QFileInfoList logFiles = logsDir.entryInfoList({"*_" + AppConfig::BACKUP_LOGS_FILE}, QDir::Files);

    for (const QFileInfo &logFile : logFiles) {
        QJsonObject metadata;
        if (JsonManager::loadJsonFile(logFile.absoluteFilePath(), metadata)) {
            totalSize += metadata.value(BackupMetadataKeys::SIZE_BYTES).toVariant().toULongLong();
        }
    }
    return totalSize;
}

// Generates backup metadata
QJsonObject BackupService::createBackupMetadata(const QString &backupFolderPath, const QStringList &selectedItems, qint64 backupDuration) const {
    QJsonArray filesArray, foldersArray, userItemsArray;
    QSet<QString> uniqueFiles, uniqueFolders;

    for (const QString &item : selectedItems) {
        userItemsArray.append(item);
        QFileInfo fileInfo(item);
        if (fileInfo.isDir()) {
            FileOperations::collectDirectoriesRecursively(item, uniqueFolders, foldersArray);
            FileOperations::collectFilesRecursively(item, uniqueFiles, filesArray);
        } else if (!uniqueFiles.contains(item)) {
            uniqueFiles.insert(item);
            filesArray.append(item);
        }
    }

    const qint64 totalSize = calculateTotalBackupSize(selectedItems);

    return QJsonObject{
        {BackupMetadataKeys::NAME, QFileInfo(backupFolderPath).fileName()},
        {BackupMetadataKeys::TIMESTAMP, Utils::Formatting::formatTimestamp(QDateTime::currentDateTime(), Qt::ISODate)},
        {BackupMetadataKeys::DURATION, backupDuration},
        {BackupMetadataKeys::DURATION_READABLE, Utils::Formatting::formatDuration(backupDuration)},
        {BackupMetadataKeys::SIZE_BYTES, totalSize},
        {BackupMetadataKeys::SIZE_READABLE, Utils::Formatting::formatSize(totalSize)},
        {BackupMetadataKeys::FILE_COUNT, filesArray.size()},
        {BackupMetadataKeys::FOLDER_COUNT, uniqueFolders.size()},
        {BackupMetadataKeys::USER_SELECTED_ITEMS, userItemsArray},
        {BackupMetadataKeys::USER_SELECTED_ITEM_COUNT, selectedItems.size()},
        {BackupMetadataKeys::BACKUP_FILES, filesArray},
        {BackupMetadataKeys::BACKUP_FOLDERS, foldersArray}
    };
}
