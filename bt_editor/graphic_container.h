#ifndef GRAPHIC_CONTAINER_H
#define GRAPHIC_CONTAINER_H

#include <QObject>
#include <QWidget>

#include "bt_editor_base.h"
#include "editor_flowscene.h"

#include <nodes/Node>
#include <nodes/NodeData>
#include <nodes/FlowScene>
#include <nodes/DataModelRegistry>
#include <nodes/FlowView>


class GraphicContainer : public QObject
{
    Q_OBJECT
public:
    explicit GraphicContainer(std::shared_ptr<QtNodes::DataModelRegistry> registry,
                              QWidget *parent = nullptr);

    EditorFlowScene* scene() { return _scene; }
    QtNodes::FlowView*  view() { return _view; }


    const EditorFlowScene* scene()  const{ return _scene; }
    const QtNodes::FlowView*  view() const { return _view; }

    void lockEditing(bool locked);

    void nodeReorder();

    void zoomHomeView();

    bool containsValidTree() const;

    void clearScene();

    AbsBehaviorTree& loadedTree() { return _abstract_tree; }

    const AbsBehaviorTree& loadedTree() const { return _abstract_tree; }

    void loadSceneFromTree(const AbsBehaviorTree &tree);

    void appendTreeToNode(QtNodes::Node& node, const AbsBehaviorTree &tree);

    void loadFromJson(const QByteArray& data);

signals:

    void undoableChange();

    void requestSubTreeAppend(GraphicContainer& container,
                              QtNodes::Node& node);

public slots:

    void onNodeDoubleClicked(QtNodes::Node& root_node);

    void onNodeCreated(QtNodes::Node &node);

    void onNodeContextMenu(QtNodes::Node& node, const QPointF& pos);

    void onConnectionContextMenu(QtNodes::Connection &connection, const QPointF&);


private:
    EditorFlowScene* _scene;
    QtNodes::FlowView*  _view;

    void createMorphSubMenu(QtNodes::Node &node, QMenu *nodeMenu);

   void createSmartRemoveAction(QtNodes::Node &node, QMenu *nodeMenu);

   void insertNodeInConnection(QtNodes::Connection &connection, QString node_name);

   void recursiveLoadStep(QPointF &cursor, double &x_offset,
                          AbstractTreeNode* abs_node,
                          QtNodes::Node* parent_node, int nest_level);

   std::shared_ptr<QtNodes::DataModelRegistry> _model_registry;

   bool _signal_was_blocked;

   AbsBehaviorTree _abstract_tree;

};

#endif // GRAPHIC_CONTAINER_H
