#ifndef LABELS_SETTINGS_H
#define LABELS_SETTINGS_H

#include <QString>

// Backup UI labels
namespace Labels::Backup {
constexpr auto k_STATUS_LIGHT_IMAGE_FORMAT = "PNG";
constexpr auto k_FOUND = "Backup Found: ";
constexpr auto k_LOCATION = "Backup Location: ";
constexpr auto k_TOTAL_COUNT = "Backup Total Count: ";
constexpr auto k_TOTAL_SIZE = "Backup Total Size: ";
constexpr auto k_LOCATION_ACCESS = "Backup Location Access: ";
}

// Last backup UI labels
namespace Labels::LastBackup {
constexpr auto k_NAME = "Last Backup Name: ";
constexpr auto k_TIMESTAMP = "Last Backup Timestamp: ";
constexpr auto k_DURATION = "Last Backup Duration: ";
constexpr auto k_SIZE = "Last Backup Size: ";
}

// Toolbar UI labels
namespace Labels::Toolbar {
constexpr auto k_SETTINGS = "Settings";
constexpr auto k_EXIT = "Exit";
constexpr auto k_HELP = "Help";
constexpr auto k_ABOUT = "About";
}

// Directory write status labels
namespace Labels::DirectoryStatus {
constexpr auto k_WRITABLE = "Writable";
constexpr auto k_READ_ONLY = "Read-Only";
constexpr auto k_UNKNOWN = "Unknown";
}

#endif // LABELS_SETTINGS_H
