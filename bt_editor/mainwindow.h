#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <nodes/Node>
#include <QTreeWidgetItem>
#include <QShortcut>
#include <QTimer>
#include <deque>
#include <thread>
#include <mutex>
#include <nodes/DataModelRegistry>

#ifdef USING_ROS
#include <ros/ros.h>
#include <std_msgs/String.h>
#endif
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

    void onNodeMoved();

    void onNodeSizeChanged();

    void onSceneChanged();

    virtual void closeEvent(QCloseEvent *event) override;

    void on_selectMode_sliderPressed();

    void on_selectMode_valueChanged(int value);

    void onTimerUpdate();

    void onNodeContextMenu(QtNodes::Node& n, const QPointF& pos);

    void onConnectionContextMenu(QtNodes::Connection& n, const QPointF& pos);

    void on_splitter_splitterMoved(int pos = 0, int index = 0);

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

 //   void recursivelyCreateXml(QDomDocument& doc, QDomElement& parent_element, const QtNodes::Node* node);

    QTimer _periodic_timer;

    std::mutex _mutex;

    void buildTreeView();


    bool    _state_received;
    QString _state_msg;
    QtNodes::Node* _root_node;

    QMap<QString, QTreeWidgetItem*> _tree_view_top_level_items;

//    QDomDocument _domDocument;

};

#endif // MAINWINDOW_H
