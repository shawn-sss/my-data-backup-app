#ifndef CUSTOMTITLEBAR_H
#define CUSTOMTITLEBAR_H

// Project includes same directory
#include "customtitlebarmode.h"

// Built-in Qt includes
#include <QPointer>
#include <QWidget>

// Forward declarations Qt classes
class QLabel;
class QPushButton;

// Custom title bar for application windows
class CustomTitleBar : public QWidget {
    Q_OBJECT

public:
    // Constructor
    explicit CustomTitleBar(QWidget *parent = nullptr);

    // Provides size hints for layout management
    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

signals:
    // Signals for minimizing and closing the window
    void minimizeRequested();
    void closeRequested();

private:
    // Initializes the layout and components
    void setupLayout();

    // Creates a styled button for the title bar
    QPushButton* createButton(const QString &label, const QString &style, const QString &tooltip);

    // UI components
    QLabel *titleLabel;
    QPushButton *minimizeButton;
    QPushButton *closeButton;
};

// Configures and returns a custom title bar for a given window
QPointer<CustomTitleBar> setupCustomTitleBar(QWidget *window, TitleBarMode mode);

#endif // CUSTOMTITLEBAR_H
