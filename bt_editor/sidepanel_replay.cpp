#include "sidepanel_replay.h"
#include "ui_sidepanel_replay.h"

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
