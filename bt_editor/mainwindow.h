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

#include "nodes/FlowScene"
#include "XML_utilities.hpp"
#include "node_palette.h"

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

    void on_actionZoom_In_triggered();

    void on_actionZoom_ut_triggered();

    void on_actionAuto_arrange_triggered();

    void onNodeCreated(QtNodes::Node &node);

    void onNodeSizeChanged();

    void onSceneChanged();

    virtual void closeEvent(QCloseEvent *event) override;

    void on_selectMode_sliderPressed();

    void on_selectMode_valueChanged(int value);

    void onTimerUpdate();

    void onNodeContextMenu(QtNodes::Node& n, const QPointF& pos);

    void onConnectionContextMenu(QtNodes::Connection& connection, const QPointF& pos);

    void on_splitter_splitterMoved(int pos = 0, int index = 0);

    void onPushUndo();

    void onUndoInvoked();

    void onRedoInvoked();

    void onNodeParameterUpdated(QString, QWidget*);

signals:
    void updateGraphic();

private:
    Ui::MainWindow *ui;

    void lockEditing(const bool locked);

    bool eventFilter(QObject *obj, QEvent *event) override;

    void resizeEvent(QResizeEvent *) override;

   // void updateStates(QXmlInputSource* source);

    std::shared_ptr<QtNodes::DataModelRegistry> _model_registry;

    struct TabInfo
    {
      QtNodes::FlowScene* scene;
      QtNodes::FlowView*  view;
    };

    std::map<QString, TabInfo> _tab_info;

    TabInfo* currentTabInfo();

    void createTab(const QString &name);

    bool _node_moved;

    QShortcut _arrange_shortcut;

    QTimer _periodic_timer;

    std::mutex _mutex;

    bool    _state_received;
    QString _state_msg;
    QtNodes::Node* _root_node;

     void createMorphSubMenu(QtNodes::Node &node, QMenu *nodeMenu);

    void createSmartRemoveAction(QtNodes::Node &node, QMenu *nodeMenu);

    void insertNodeInConnection(QtNodes::Connection &connection, QString node_name);

    std::deque<QByteArray> _undo_stack;
    std::deque<QByteArray> _redo_stack;
    QByteArray _current_state;
    std::atomic_bool _undo_enabled;

    TreeNodeModels _tree_nodes_model;

    NodePalette* _node_palette_widget;

};



#endif // MAINWINDOW_H
