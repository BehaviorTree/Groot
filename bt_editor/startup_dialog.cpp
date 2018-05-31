#include "startup_dialog.h"
#include "ui_startup_dialog.h"

#include <QSettings>
#include <QShortcut>

StartupDialog::StartupDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::StartupDialog),
    _mode(GraphicMode::EDITOR)
{
    ui->setupUi(this);

    QSettings settings("EurecatRobotics", "BehaviorTreeEditor");
    QString mode  = settings.value("StartupDialog.Mode", "EDITOR").toString();

    if( mode == "EDITOR")
    {
        _mode = GraphicMode::EDITOR;
    }
    else if( mode == "MONITOR")
    {
        _mode = GraphicMode::MONITOR;
    }
    else if( mode == "REPLAY")
    {
        _mode = GraphicMode::REPLAY;
    }
    updateCurrentMode();

    QShortcut* close_shortcut = new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_C), this);
    connect( close_shortcut, &QShortcut::activated, this, &QDialog::reject );
}

StartupDialog::~StartupDialog()
{
    delete ui;
}

void StartupDialog::on_toolButtonEditor_clicked()
{
    _mode = GraphicMode::EDITOR;
    updateCurrentMode();
}

void StartupDialog::on_toolButtonMonitor_clicked()
{
    _mode = GraphicMode::MONITOR;
    updateCurrentMode();
}

void StartupDialog::on_toolButtonReplay_clicked()
{
    _mode = GraphicMode::REPLAY;
    updateCurrentMode();
}

void StartupDialog::updateCurrentMode()
{
    const QString selected_style( "color:white; "
                                  "border: 2px solid orange; border-radius: 10px;"
                                  "background-color: rgb(190, 101, 0);" );

    const QString default_style( "QToolButton {"
                                 "color:white; "
                                 "border: 2px solid orange; border-radius: 10px;"
                                 "background-color: rgba(0, 0, 0, 0);"
                                 "}\n"
                                 "QToolButton:hover {color:white; background-color: rgb(111, 111, 111);}");

    if( _mode == GraphicMode::EDITOR)
    {
            ui->labelDescription->setText("Create/Load/Edit/Save your own BehaviorTree.");
    }
    else if( _mode == GraphicMode::MONITOR)
    {
        ui->labelDescription->setText("Connect to a remote server to visualize the status"
                                      " of a BehaviorTree in real-time.");
    }
    else if( _mode == GraphicMode::REPLAY)
    {
        ui->labelDescription->setText("Load a BehaviorTree Log file to visualize all the"
                                      " transitions off-line.");
    }

    ui->toolButtonEditor->setStyleSheet(  _mode == GraphicMode::EDITOR ? selected_style : default_style);
    ui->toolButtonMonitor->setStyleSheet( _mode == GraphicMode::MONITOR ? selected_style : default_style);
    ui->toolButtonReplay->setStyleSheet(  _mode == GraphicMode::REPLAY ? selected_style : default_style);
}

void StartupDialog::on_toolButtonStart_clicked()
{
    QSettings settings("EurecatRobotics", "BehaviorTreeEditor");

    if( _mode == GraphicMode::EDITOR)
    {
        settings.setValue("StartupDialog.Mode", "EDITOR");
    }
    else if( _mode == GraphicMode::MONITOR)
    {
        settings.setValue("StartupDialog.Mode", "MONITOR");
    }
    else if( _mode == GraphicMode::REPLAY)
    {
        settings.setValue("StartupDialog.Mode", "REPLAY");
    }
    this->accept();
}
