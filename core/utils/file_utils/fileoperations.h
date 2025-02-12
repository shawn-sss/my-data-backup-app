#ifndef FILEOPERATIONS_H
#define FILEOPERATIONS_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QDir>

// File operations utilities
namespace FileOperations {

// Directory and file management
bool copyDirectoryRecursively(const QString &source, const QString &destination);
quint64 calculateDirectorySize(const QDir &dir);
bool createDirectory(const QString &path);
bool deleteDirectory(const QString &path);

// JSON file handling
bool writeJsonToFile(const QString &filePath, const QJsonObject &jsonObject);
QJsonObject readJsonFromFile(const QString &filePath);

// File collection and traversal
void collectFilesRecursively(const QString &dirPath, QSet<QString> &uniqueFiles, QJsonArray &filesArray);

// Backup infrastructure setup
bool createBackupInfrastructure(const QString &backupDir, QString &errorMessage);

} // namespace FileOperations

#endif // FILEOPERATIONS_H
