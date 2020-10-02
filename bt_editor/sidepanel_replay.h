#ifndef SIDEPANEL_REPLAY_H
#define SIDEPANEL_REPLAY_H

#include <chrono>
#include <QFrame>
#include <QTableWidgetItem>
#include <QStandardItemModel>
#include "bt_editor_base.h"


namespace Ui {
class SidepanelReplay;
}

class SidepanelReplay : public QFrame
{
    Q_OBJECT

public:
    explicit SidepanelReplay(QWidget *parent = 0);
    ~SidepanelReplay() override;

    void clear();

    void loadLog(const QByteArray& content);

    size_t transitionsCount() const { return _transitions.size(); }

public slots:

    void on_LoadLog();

private slots:

    void on_pushButtonPlay_toggled(bool checked);

    void on_spinBox_valueChanged(int arg1);

    void on_timeSlider_valueChanged(int value);

    void on_tableView_clicked(const QModelIndex &index);

    void onTimerUpdate();

    void onPlayUpdate();

    void on_lineEditFilter_textChanged(const QString &filter_text);

signals:
    void loadBehaviorTree(const AbsBehaviorTree& tree, const QString& name );

    void changeNodeStyle(const QString& bt_name,
                         const std::vector<std::pair<int, NodeStatus>>& node_status);

    void addNewModel(const NodeModel &new_model);

private:

    bool eventFilter(QObject *object, QEvent *event) override;

    void loadFromFlatbuffers(const std::vector<int8_t>& serialized_description);

    void onRowChanged(int value);

    Ui::SidepanelReplay *ui;

    struct Transition{
        int16_t index;
        double timestamp;
        NodeStatus prev_status;
        NodeStatus status;
        bool is_tree_restart;
        int nearest_restart_transition_index;
    };
    std::vector<Transition> _transitions;
    std::vector< std::pair<double,int>> _timepoint;

    int _prev_row;
    int _next_row;

    void updatedSpinAndSlider(int row);

    QStandardItemModel* _table_model;

    QTimer *_layout_update_timer;

    QTimer *_play_timer;

    AbsBehaviorTree _loaded_tree;

    void updateTableModel(const AbsBehaviorTree &tree);

    QWidget *_parent;
};

#endif // SIDEPANEL_REPLAY_H
