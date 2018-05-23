#ifndef SIDEPANEL_REPLAY_H
#define SIDEPANEL_REPLAY_H

#include <QFrame>
#include <QTableWidgetItem>
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

    void on_tableWidget_itemClicked(QTableWidgetItem *item);

signals:
    void loadBehaviorTree( AbsBehaviorTree& tree );

    void repaint();

private:

    bool eventFilter(QObject *object, QEvent *event) override;

    void loadFromFlatbuffers(const std::vector<int8_t>& serialized_description);

    void onValueChanged(int value);

    Ui::SidepanelReplay *ui;

    struct Transition{
        uint16_t uid;
        double timestamp;
        NodeStatus prev_status;
        NodeStatus status;
    };
    std::vector<Transition> _transitions;
    std::vector< std::pair<double,int>> _timepoint;

    int _prev_row;

    AbsBehaviorTree _loaded_tree;
    void updatedSpinAndSlider(int row);
};

#endif // SIDEPANEL_REPLAY_H
