#include "transferworker.h"
#include "../../utils/file_utils/fileoperations.h"
#include "../../config/_constants.h"

#include <QStringList>
#include <QFile>
#include <QDir>
#include <QStorageInfo>

// Constructor & Initialization
TransferWorker::TransferWorker(const QStringList &files, const QString &destination, QObject *parent)
    : QObject(parent), files(files), destination(destination) {}

// Transfer Control
void TransferWorker::stopTransfer() {
    stopRequested = true;
}

void TransferWorker::startTransfer() {
    int totalFiles = files.size();
    if (totalFiles == 0) {
        emit transferComplete();
        emit finished();
        return;
    }

    int completedFiles = 0;
    for (const QString &filePath : files) {
        if (stopRequested) {
            emit errorOccurred(ErrorMessages::WARNING_OPERATION_STILL_RUNNING);
            return;
        }

        QFileInfo fileInfo(filePath);
        bool success = fileInfo.isDir() && filePath.endsWith(":/")
                           ? processDriveRoot(filePath)
                           : processFileOrFolder(filePath);

        if (!success) {
            return;
        }

        completedFiles++;
        emit progressUpdated((completedFiles * 100) / totalFiles);
    }

    emit transferComplete();
    emit finished();
}

// File Processing
bool TransferWorker::processDriveRoot(const QString &driveRoot) {
    if (stopRequested) return false;

    QFileInfo fileInfo(driveRoot);
    if (!fileInfo.exists() || !fileInfo.isDir()) {
        emit errorOccurred(QString(ErrorMessages::ERROR_NO_ITEMS_SELECTED_FOR_BACKUP).arg(driveRoot));
        return false;
    }

    QString driveLetter = driveRoot.left(1);
    QStorageInfo storageInfo(driveRoot);
    QString driveLabel = storageInfo.displayName().isEmpty() ? BackupInfo::DEFAULT_DRIVE_LABEL : storageInfo.displayName();

    QString driveName = QString("%1 (%2 %3)").arg(driveLabel, driveLetter, BackupInfo::DRIVE_LABEL_SUFFIX);
    QString driveBackupFolder = QDir(destination).filePath(driveName);

    QDir dir(driveBackupFolder);
    if (dir.exists()) {
        dir.removeRecursively();
    }

    if (!QDir().mkpath(driveBackupFolder)) {
        emit errorOccurred(ErrorMessages::ERROR_CREATING_BACKUP_FOLDER);
        return false;
    }

    QDir driveDir(driveRoot);
    const QFileInfoList entries = driveDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files);

    if (stopRequested) return false;

    for (int i = 0; i < entries.size(); ++i) {
        const QFileInfo &entry = entries.at(i);
        QString destPath = driveBackupFolder + QDir::separator() + entry.fileName();
        if (!copyItem(entry, destPath)) {
            return false;
        }
    }

    return true;
}

bool TransferWorker::processFileOrFolder(const QString &filePath) {
    if (stopRequested) return false;

    QFileInfo fileInfo(filePath);
    QString destinationPath = QDir(destination).filePath(fileInfo.fileName());

    return copyItem(fileInfo, destinationPath);
}

// Helper Methods
bool TransferWorker::copyItem(const QFileInfo &fileInfo, const QString &destinationPath) {
    if (!fileInfo.isReadable()) {
        emit errorOccurred(QString(ErrorMessages::ERROR_FILE_ACCESS_DENIED).arg(fileInfo.absoluteFilePath()));
        return false;
    }

    if (QFile::exists(destinationPath) && !QFile::remove(destinationPath)) {
        emit errorOccurred(QString(ErrorMessages::ERROR_TRANSFER_FAILED).arg(destinationPath));
        return false;
    }

    bool success = fileInfo.isDir()
                       ? FileOperations::copyDirectoryRecursively(fileInfo.absoluteFilePath(), destinationPath)
                       : QFile::copy(fileInfo.absoluteFilePath(), destinationPath);

    if (!success) {
        emit errorOccurred(QString(ErrorMessages::ERROR_TRANSFER_FAILED).arg(fileInfo.absoluteFilePath()));
        return false;
    }

    return true;
}
