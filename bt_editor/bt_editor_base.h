#ifndef BT_EDITOR_BASE_H
#define BT_EDITOR_BASE_H

#include <QString>
#include <QPointF>
#include <QSizeF>
#include <map>
#include <unordered_map>
#include <nodes/Node>
#include <deque>

enum class NodeType   { ACTION, DECORATOR, CONTROL, CONDITION, SUBTREE, ROOT, UNDEFINED };
enum class NodeStatus { IDLE, RUNNING, SUCCESS, FAILURE };
enum class GraphicMode { EDITOR, MONITOR, REPLAY };


NodeType getNodeTypeFromString(const QString& str);

GraphicMode getGraphicModeFromString(const QString& str);

const char* toStr(NodeStatus type);

const char* toStr(NodeType type);

const char* toStr(GraphicMode type);

struct ParameterWidgetCreator{
    QString label;
    std::function<QWidget*()> instance_factory;
};

using ParameterWidgetCreators = std::vector<ParameterWidgetCreator>;


struct TreeNodeModel
{
    NodeType type;
    struct Param
    {
        QString label;
        QString value;
    };

    QString registration_ID;
    typedef std::vector<Param> Parameters;
    Parameters params;

    TreeNodeModel(QString reg_name, NodeType type, const std::vector<Param>& parameters ):
        type(type),
        registration_ID( std::move(reg_name) ),
        params(parameters)
    {}

    bool operator ==(const TreeNodeModel& other) const;

    bool operator !=(const TreeNodeModel& other) const
    {
        return !(*this == other);
    }
};


typedef std::map<QString, TreeNodeModel> TreeNodeModels;

inline TreeNodeModels& BuiltinNodeModels()
{
    static TreeNodeModels builtin_node_models;
    return builtin_node_models;
}

//--------------------------------
struct AbstractTreeNode
{
    AbstractTreeNode() :
        model( { "", NodeType::UNDEFINED, {} }),
        index(-1),
        status(NodeStatus::IDLE),
        graphic_node(nullptr) {}

    TreeNodeModel model;
    int index;
    QString instance_name;
    NodeStatus status;
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
