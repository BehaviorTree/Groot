#ifndef STARTUP_DIALOG_H
#define STARTUP_DIALOG_H

#include <QDialog>
#include "bt_editor_base.h"

namespace Ui {
class StartupDialog;
}

class StartupDialog : public QDialog
{
    Q_OBJECT

public:
    explicit StartupDialog(QWidget *parent = 0);
    ~StartupDialog();

    GraphicMode getGraphicMode() const { return _mode; }

private slots:
    void on_toolButtonEditor_clicked();

    void on_toolButtonMonitor_clicked();

    void on_toolButtonReplay_clicked();

    void updateCurrentMode();

    void on_toolButtonStart_clicked();

private:
    Ui::StartupDialog *ui;

    GraphicMode _mode;
};

#endif // STARTUP_DIALOG_H
