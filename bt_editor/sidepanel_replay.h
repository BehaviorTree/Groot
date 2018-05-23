#ifndef SIDEPANEL_REPLAY_H
#define SIDEPANEL_REPLAY_H

#include <QFrame>
#include "bt_editor_base.h"

namespace Ui {
class SidepanelReplay;
}

class SidepanelReplay : public QFrame
{
    Q_OBJECT

public:
    explicit SidepanelReplay(QWidget *parent = 0);
    ~SidepanelReplay();

private slots:
    void on_pushButtonLoadLog_pressed();

    void on_pushButtonPlay_toggled(bool checked);

    void on_spinBox_valueChanged(int arg1);

    void on_timeSlider_valueChanged(int value);

signals:
    void loadBehaviorTree( AbsBehaviorTree tree );

private:

    void loadFromFlatbuffers(const std::vector<int8_t>& serialized_description);

    Ui::SidepanelReplay *ui;
};

#endif // SIDEPANEL_REPLAY_H
