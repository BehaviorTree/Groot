#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include <nodes/Node>
#include <QTreeWidgetItem>
#include <QShortcut>
#include <QTimer>
#include <deque>
#include <thread>
#include <mutex>
#include <nodes/DataModelRegistry>

#include "graphic_container.h"
#include "XML_utilities.hpp"
#include "sidepanel_editor.h"
#include "sidepanel_replay.h"

namespace Ui {
class MainWindow;
}

namespace QtNodes{
class FlowView;
class FlowScene;
class Node;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

  void loadFromXML(const QString &xml_text);

private slots:

    void on_actionLoad_triggered();

    void on_actionSave_triggered();

    void on_actionZoom_Out_triggered();

    void on_actionZoom_In_triggered();

    void on_actionAuto_arrange_triggered();

    void onSceneChanged();

    virtual void closeEvent(QCloseEvent *event) override;

    void on_splitter_splitterMoved(int pos = 0, int index = 0);

    void onPushUndo();

    void onUndoInvoked();

    void onRedoInvoked();

    void on_comboBoxLayout_currentIndexChanged(int index);

    void on_pushButtonReorder_pressed();

    void on_pushButtonCenterView_pressed();

    void on_radioEditor_toggled(bool checked);

    void on_radioMonitor_toggled(bool checked);

    void on_radioReplay_toggled(bool checked);

    void on_pushButtonTest_pressed();

    void on_loadBehaviorTree(AbsBehaviorTree tree);

signals:
    void updateGraphic();

private:
    Ui::MainWindow *ui;

    void lockEditing(const bool locked);

    bool eventFilter(QObject *obj, QEvent *event) override;

    void resizeEvent(QResizeEvent *) override;

    std::shared_ptr<QtNodes::DataModelRegistry> _model_registry;

    std::map<QString, GraphicContainer*> _tab_info;

    GraphicContainer* currentTabInfo();

    void createTab(const QString &name);

    std::mutex _mutex;

    QtNodes::Node* _root_node;

    std::deque<QByteArray> _undo_stack;
    std::deque<QByteArray> _redo_stack;
    QByteArray _current_state;
    bool _undo_enabled;

    TreeNodeModels _tree_nodes_model;

    SidepanelEditor* _editor_widget;
    SidepanelReplay* _replay_widget;
};



#endif // MAINWINDOW_H
