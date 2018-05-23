#include "sidepanel_replay.h"
#include "ui_sidepanel_replay.h"

#include<QDir>
#include<QFile>
#include<QFileInfo>
#include<QFileDialog>
#include<QSettings>

SidepanelReplay::SidepanelReplay(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::SidepanelReplay)
{
    ui->setupUi(this);
}

SidepanelReplay::~SidepanelReplay()
{
    delete ui;
}

void SidepanelReplay::on_pushButtonLoadLog_pressed()
{
  QSettings settings("EurecatRobotics", "BehaviorTreeEditor");
  QString directory_path  = settings.value("SidepanelReplay.lastLoadDirectory",
                                           QDir::homePath() ).toString();

  QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                  tr("Open Flow Scene"), directory_path,
                                                  tr("Flatbuffers log (*.fbl)"));
  if (!QFileInfo::exists(fileName)){
    return;
  }
  QFile file(fileName);

  if (!file.open(QIODevice::ReadOnly)){
    return;
  }

  directory_path = QFileInfo(fileName).absolutePath();
  settings.setValue("MainWindow.lastLoadDirectory", directory_path);
  settings.sync();

  QByteArray content = file.readAll();
  //--------------------------------


}

void SidepanelReplay::on_pushButtonPlay_toggled(bool checked)
{

}

void SidepanelReplay::on_spinBox_valueChanged(int arg1)
{

}

void SidepanelReplay::on_timeSlider_valueChanged(int value)
{

}
