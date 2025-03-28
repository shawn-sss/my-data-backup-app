#ifndef UI_CONFIG_H
#define UI_CONFIG_H

// Built-in Qt includes
#include <QSize>
#include <QString>

// Backup UI labels
namespace Labels {
namespace Backup {
constexpr auto STATUS_LIGHT_IMAGE_FORMAT = "PNG";
constexpr auto FOUND = "Backup Found: ";
constexpr auto LOCATION = "Backup Location: ";
constexpr auto TOTAL_COUNT = "Backup Total Count: ";
constexpr auto TOTAL_SIZE = "Backup Total Size: ";
constexpr auto LOCATION_ACCESS = "Backup Location Access: ";
}

// Last backup UI labels
namespace LastBackup {
constexpr auto NAME = "Last Backup Name: ";
constexpr auto TIMESTAMP = "Last Backup Timestamp: ";
constexpr auto DURATION = "Last Backup Duration: ";
constexpr auto SIZE = "Last Backup Size: ";
}

// Toolbar UI labels
namespace Toolbar {
constexpr auto SETTINGS = "Settings";
constexpr auto EXIT = "Exit";
constexpr auto HELP = "Help";
constexpr auto ABOUT = "About";
}
}

// Directory write status labels
namespace DirectoryStatus {
constexpr auto WRITABLE = "Writable";
constexpr auto READ_ONLY = "Read-Only";
constexpr auto UNKNOWN = "Unknown";
}

// Tree view UI settings
namespace UISettings {
namespace TreeView {
constexpr auto STAGING_COLUMN_NAME = "Name";
constexpr int START_HIDDEN_COLUMN = 1;
constexpr int DEFAULT_COLUMN_COUNT = 4;
}
}

// HTML templates for UI components
namespace HTMLTemplates {
constexpr auto STATUS_LIGHT_ICON = "<img src='data:image/png;base64,%1' style='%2'>";
constexpr auto STATUS_LABEL_HTML =
    "<div style='display:flex; align-items:center;'>"
    "<span>%1</span><span style='margin-left:4px;'>%2</span>"
    "</div>";
}

// General UI colors
namespace Colors {
constexpr auto TITLE_BAR = "#222";
constexpr auto WINDOW_BACKGROUND = "#2B2B2B";
}

// General button styling
namespace Styles {
namespace BackupViewContainer {
constexpr auto STYLE = R"(
    #BackupViewContainer {
        border-radius: 10px;
        padding: 10px;
        border: 1px solid; /* Let theme override color */
    }

    #DestinationTreeTitleLabel,
    #SourceTreeTitleLabel,
    #StagingListTitleLabel {
        font-weight: bold;
    }

    #DriveTreeView,
    #BackupStagingTreeView,
    #BackupDestinationView {
        border: 1px solid;
    }

    #MainToolBar QToolButton {
        border-radius: 16px;
        padding: 8px;
        margin: 4px;
        min-width: 40px;
        min-height: 40px;
        font-size: 14px;
        border: none;
    }

    #ChangeBackupDestinationButton,
    #DeleteBackupButton,
    #AddToBackupButton,
    #CreateBackupButton,
    #RemoveFromBackupButton {
        border: 1px solid;
        border-radius: 6px;
        padding: 6px 12px;
    }
)";
}
namespace GeneralButton {
constexpr auto STYLE =
    "QPushButton { background: #444; color: white; border-radius: 5px; padding: 5px; }"
    "QPushButton:hover { background: #666; }"
    "QPushButton:pressed { background: #888; }";
}
}

#endif // UI_CONFIG_H
