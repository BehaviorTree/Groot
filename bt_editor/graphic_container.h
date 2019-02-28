#ifndef GRAPHIC_CONTAINER_H
#define GRAPHIC_CONTAINER_H

#include <QObject>
#include <QWidget>
#include <QLineEdit>

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
    const QtNodes::FlowView* view() const { return _view; }

    void lockEditing(bool locked);

    void lockSubtreeEditing(QtNodes::Node& node, bool locked, bool change_style);

    void nodeReorder();

    void zoomHomeView();

    bool containsValidTree() const;

    void clearScene();

    AbsBehaviorTree loadedTree() const;

    void loadSceneFromTree(const AbsBehaviorTree &tree);

    void appendTreeToNode(QtNodes::Node& node, AbsBehaviorTree &subtree);

    void loadFromJson(const QByteArray& data);

    QtNodes::Node* substituteNode(QtNodes::Node* old_node, const QString& new_node_ID);

    void deleteSubTreeRecursively(QtNodes::Node& node);

    std::set<QtNodes::Node*> getSubtreeNodesRecursively(QtNodes::Node &root_node);

    void createSubtree(QtNodes::Node& root_node, QString subtree_name = QString());

public slots:

    void onNodeDoubleClicked(QtNodes::Node& root_node);

    void onPortValueDoubleClicked(QLineEdit* edit_value);

    void onNodeCreated(QtNodes::Node &node);

    void onNodeContextMenu(QtNodes::Node& node, const QPointF& pos);

    void onConnectionContextMenu(QtNodes::Connection &connection, const QPointF&);

    void onSmartRemove(QtNodes::Node* node);

signals:

    void addNewModel( const NodeModel &new_model );

    void undoableChange();

    void requestSubTreeExpand(GraphicContainer& container,
                              QtNodes::Node& node);

    void requestSubTreeCreate(AbsBehaviorTree tree, QString name);

private:
    EditorFlowScene* _scene;
    QtNodes::FlowView*  _view;

    void createMorphSubMenu(QtNodes::Node &node, QMenu *nodeMenu);

   void createSmartRemoveAction(QtNodes::Node &node, QMenu *nodeMenu);

   void insertNodeInConnection(QtNodes::Connection &connection, QString node_name);

   void recursiveLoadStep(QPointF &cursor, AbsBehaviorTree &tree,
                          AbstractTreeNode *abs_node,
                          QtNodes::Node* parent_node, int nest_level);

   std::shared_ptr<QtNodes::DataModelRegistry> _model_registry;

   bool _signal_was_blocked;

};

#endif // GRAPHIC_CONTAINER_H
