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

    void selectRowsBackground(int last_row);

    Ui::SidepanelReplay *ui;

    struct Transition{
        uint16_t uid;
        double timestamp;
        NodeStatus prev_status;
        NodeStatus status;
    };
    std::vector<Transition> _transitions;
};

#endif // SIDEPANEL_REPLAY_H
