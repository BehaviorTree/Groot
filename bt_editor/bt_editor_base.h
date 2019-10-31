#ifndef BT_EDITOR_BASE_H
#define BT_EDITOR_BASE_H

#include <QString>
#include <QPointF>
#include <QSizeF>
#include <map>
#include <unordered_map>
#include <nodes/Node>
#include <deque>
#include <behaviortree_cpp_v3/bt_factory.h>

using BT::NodeStatus;
using BT::NodeType;
using BT::PortDirection;

typedef std::map<QString, QString> PortsMapping;

// alternative type, similar to BT::PortInfo
struct PortModel
{
    PortModel(): direction(PortDirection::INOUT) {}

    QString type_name;
    PortDirection direction;
    QString description;
    QString default_value;

    PortModel& operator = (const BT::PortInfo& src);
};

typedef std::map<QString, PortModel> PortModels;

struct  NodeModel
{
    NodeType type;
    QString  registration_ID;
    PortModels ports;

    bool operator == (const NodeModel& other) const;
    bool operator != (const NodeModel& other) const
    {
        return !( *this == other);
    }

    NodeModel& operator = (const BT::TreeNodeManifest& src);
};

typedef std::map<QString, NodeModel> NodeModels;


enum class GraphicMode { EDITOR, MONITOR, REPLAY };

GraphicMode getGraphicModeFromString(const QString& str);

const char* toStr(GraphicMode type);

const NodeModels& BuiltinNodeModels();

//--------------------------------
struct AbstractTreeNode
{
    AbstractTreeNode() :
        index(-1),
        status(NodeStatus::IDLE),
        graphic_node(nullptr)
    {
        model.type = NodeType::UNDEFINED;
    }

    NodeModel model;
    PortsMapping ports_mapping;
    int index;
    QString instance_name;
    BT::NodeStatus status;
    QSizeF size;
    QPointF pos; // top left corner
    std::vector<int> children_index;
    QtNodes::Node* graphic_node;

    bool operator ==(const AbstractTreeNode& other) const;

    bool operator !=(const AbstractTreeNode& other) const
    {
        return !(*this == other);
    }
};

class AbsBehaviorTree
{
public:

    typedef std::deque<AbstractTreeNode> NodesVector;

    AbsBehaviorTree() {}

    ~AbsBehaviorTree();

    size_t nodesCount() const {
        return _nodes.size();
    }

    const NodesVector& nodes() const { return _nodes; }

    NodesVector& nodes() { return _nodes; }

    const AbstractTreeNode* node(size_t index) const { return &_nodes.at(index); }

    AbstractTreeNode* node(size_t index) { return &_nodes.at(index); }

    AbstractTreeNode* rootNode();

    const AbstractTreeNode* rootNode() const;

    std::vector<const AbstractTreeNode*> findNodes(const QString& instance_name);

    const AbstractTreeNode* findFirstNode(const QString& instance_name);

    AbstractTreeNode* addNode(AbstractTreeNode* parent, AbstractTreeNode &&new_node );

    void debugPrint() const;

    bool operator ==(const AbsBehaviorTree &other) const;

    bool operator !=(const AbsBehaviorTree &other) const{
        return !(*this == other);
    }

    void clear();

private:
    NodesVector _nodes;
};

static int GetUID()
{
    static int uid = 1000;
    return uid++;
}

Q_DECLARE_METATYPE(AbsBehaviorTree);




#endif // BT_EDITOR_BASE_H
