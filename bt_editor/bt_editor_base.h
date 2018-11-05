#ifndef BT_EDITOR_BASE_H
#define BT_EDITOR_BASE_H

#include <QString>
#include <QPointF>
#include <QSizeF>
#include <map>
#include <unordered_map>
#include <nodes/Node>

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
    NodeType node_type;
    bool is_editable;
    struct Param
    {
        QString label;
        QString default_value;
    };

    typedef std::vector<Param> Parameters;

    Parameters params;

    TreeNodeModel():is_editable(false){}

    TreeNodeModel(NodeType type, const std::vector<Param>& parameters ):
        node_type(type),
        is_editable(false),
        params(parameters)
    {}
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
    AbstractTreeNode() : index(-1), corresponding_node(nullptr) {}

    QString registration_name;
    QString instance_name;
    NodeType type;
    NodeStatus status;
    QSizeF size;
    QPointF pos; // top left corner
    int16_t index;
    std::vector<int16_t> children_index;
    QtNodes::Node* corresponding_node;
    std::vector< std::pair<QString,QString> > parameters;
};

class AbsBehaviorTree
{
public:
    AbsBehaviorTree():_root_node_index(-1) {}

    size_t nodesCount() const {
        return _nodes.size();
    }

    const std::vector<AbstractTreeNode>& nodes() const { return _nodes; }

    std::vector<AbstractTreeNode>& nodes() { return _nodes; }

    AbstractTreeNode* rootNode();

    AbstractTreeNode* nodeAtIndex( int16_t index ) {
        return &_nodes.at( index );
    }

    AbstractTreeNode* nodeAtUID( uint16_t uid ) {
        return &_nodes.at( UidToIndex(uid) );
    }

    int rootIndex() const
    {
      return _root_node_index;
    }

    const AbstractTreeNode* rootNode() const;

    const AbstractTreeNode* nodeAtIndex( int16_t index ) const{
        return &_nodes.at( index );
    }

    const AbstractTreeNode* nodeAtUID( uint16_t uid ) const{
        return &_nodes.at( UidToIndex(uid) );
    }

    void pushBack( uint16_t UID, AbstractTreeNode node );

    int UidToIndex(uint16_t uid) const ;

    void updateRootIndex();

    void debugPrint();

private:
    std::vector<AbstractTreeNode> _nodes;
    std::unordered_map<uint16_t, int16_t> _UID_to_index;
    int16_t _root_node_index;
};

static int GetUID()
{
    static int uid = 1000;
    return uid++;
}

Q_DECLARE_METATYPE(AbsBehaviorTree);




#endif // BT_EDITOR_BASE_H
