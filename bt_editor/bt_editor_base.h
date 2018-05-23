#ifndef BT_EDITOR_BASE_H
#define BT_EDITOR_BASE_H

#include <QString>
#include <QPointF>
#include <QSizeF>
#include <map>
#include <unordered_map>
#include <nodes/Node>

enum class NodeType   { ACTION, DECORATOR, CONTROL, CONDITION, SUBTREE, ROOT, UNDEFINED };
enum class ParamType  { INT, DOUBLE, TEXT, COMBO, UNDEFINED };
enum class NodeStatus { IDLE, RUNNING, SUCCESS, FAILURE };

struct TreeNodeModel
{
  NodeType node_type;
  std::map<QString, ParamType> params;
};

typedef std::map<QString, TreeNodeModel> TreeNodeModels;
//--------------------------------
struct AbstractTreeNode
{
    AbstractTreeNode() : uid(0), corresponding_node(nullptr) {}

    QString registration_name;
    QString instance_name;
    NodeType type;
    NodeStatus status;
    QSizeF size;
    QPointF pos; // top left corner
    uint16_t uid;
    uint8_t _padding_[6];
    std::vector<AbstractTreeNode*> children;
    QtNodes::Node* corresponding_node;
    std::vector< std::pair<QString,QString> > parameters;
};

struct AbsBehaviorTree
{
    AbsBehaviorTree():root_node_uid(0) {}

    std::map<uint16_t, AbstractTreeNode> nodes;
    uint16_t root_node_uid;
    uint8_t _padding_[6];
};

Q_DECLARE_METATYPE(AbsBehaviorTree);




#endif // BT_EDITOR_BASE_H
