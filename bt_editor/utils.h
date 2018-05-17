#ifndef NODE_UTILS_H
#define NODE_UTILS_H

#include <nodes/NodeData>
#include <nodes/FlowScene>


std::vector<QtNodes::Node*> findRoots(const QtNodes::FlowScene &scene);

std::vector<QtNodes::Node *> getChildren(const QtNodes::FlowScene &scene,
                                         const QtNodes::Node &parent_node);

struct AbstractGeometry
{
  QString name;
  QPointF pos; // top left corner
  QSizeF size;
  std::vector<AbstractGeometry*> children;
};

typedef std::map<QtNodes::Node*, AbstractGeometry> AbstractTree;

AbstractTree BuildAbstractTree(QtNodes::FlowScene &scene);

void NodeReorder(QtNodes::FlowScene &scene, AbstractTree abstract_tree );


QString getCategory(const QtNodes::NodeDataModel* model);

#endif // NODE_UTILS_H
