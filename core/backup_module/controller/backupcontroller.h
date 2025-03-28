#ifndef BACKUPCONTROLLER_H
#define BACKUPCONTROLLER_H

// Built-in Qt includes
#include <QObject>
#include <QStringList>

// Forward declarations Qt classes
class QProgressBar;
class QThread;

// Forward declarations custom classes
class BackupService;

// Manages backup operations
class BackupController : public QObject {
    Q_OBJECT

public:
    // Constructor & Destructor
    explicit BackupController(BackupService *service, QObject *parent = nullptr);
    ~BackupController();

    // Backup management
    void createBackup(const QString &destinationPath, const QStringList &stagingList, QProgressBar *progressBar);
    void deleteBackup(const QString &backupPath);
    bool isBackupInProgress() const;

signals:
    // Signals for backup status
    void backupCreated();
    void backupDeleted();
    void errorOccurred(const QString &error);

private:
    // Internal helper functions
    void cleanupAfterTransfer();
    bool createBackupFolder(const QString &path);

    // Internal state
    BackupService *backupService;
    QThread *workerThread;
};

#endif // BACKUPCONTROLLER_H
