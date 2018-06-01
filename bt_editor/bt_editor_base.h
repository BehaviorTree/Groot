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
enum class GraphicMode { EDITOR, MONITOR, REPLAY };


inline
ParamType getParamTypeFromString(const QString& str)
{
  if( str == "Text")  return ParamType::TEXT;
  if( str == "Int")    return ParamType::INT;
  if( str == "Double") return ParamType::DOUBLE;
  if( str == "Combo")  return ParamType::COMBO;
  return ParamType::UNDEFINED;
};

inline NodeType getNodeTypeFromString(const QString& str)
{
  if( str == "Action")    return NodeType::ACTION;
  if( str == "Decorator") return NodeType::DECORATOR;
  if( str == "Condition") return NodeType::CONDITION;
  if( str == "SubTree")   return NodeType::SUBTREE;
  if( str == "Control")   return NodeType::CONTROL;
  if( str == "Root")   return NodeType::ROOT;
  return NodeType::UNDEFINED;
};

inline GraphicMode getGraphicModeFromString(const QString& str)
{
  if( str == "EDITOR")
      return GraphicMode::EDITOR;
  else if( str == "MONITOR")
      return GraphicMode::MONITOR;
  return GraphicMode::REPLAY;
};


inline const char* toStr(NodeType type)
{
  if( type == NodeType::ACTION )   return "Action";
  if( type == NodeType::DECORATOR) return "Decorator";
  if( type == NodeType::CONDITION) return "Decorator";
  if( type == NodeType::SUBTREE)   return "SubTree";
  if( type == NodeType::CONTROL)   return "Control";
  if( type == NodeType::CONTROL)   return "Root";
  return nullptr;
};

inline const char* toStr(ParamType type)
{
  if( type == ParamType::TEXT)   return "Text";
  if( type == ParamType::INT )   return "Int";
  if( type == ParamType::DOUBLE) return "Double";
  if( type == ParamType::COMBO)  return "Combo";
  return nullptr;
};

inline const char* toStr(GraphicMode type)
{
  if( type == GraphicMode::EDITOR)   return "EDITOR";
  if( type == GraphicMode::MONITOR ) return "MONITOR";
  if( type == GraphicMode::REPLAY)   return "REPLAY";
  return nullptr;
};


struct TreeNodeModel
{
  NodeType node_type;
  std::map<QString, ParamType> params;
};

typedef std::map<QString, TreeNodeModel> TreeNodeModels;
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

    AbstractTreeNode* rootNode();

    AbstractTreeNode& nodeAtIndex( int16_t index ) {
        return _nodes.at( index );
    }

    AbstractTreeNode& nodeAtUID( uint16_t uid ) {
        return _nodes.at( UidToIndex(uid) );
    }

    const AbstractTreeNode* rootNode() const;

    const AbstractTreeNode& nodeAtIndex( int16_t index ) const{
        return _nodes.at( index );
    }

    const AbstractTreeNode& nodeAtUID( uint16_t uid ) const{
        return _nodes.at( UidToIndex(uid) );
    }

    void pushBack( uint16_t UID, AbstractTreeNode node );

    int UidToIndex(uint16_t uid) const ;

    void updateRootIndex();

private:
    std::vector<AbstractTreeNode> _nodes;
    std::unordered_map<uint16_t, int16_t> _UID_to_index;
    int16_t _root_node_index;
};

Q_DECLARE_METATYPE(AbsBehaviorTree);




#endif // BT_EDITOR_BASE_H
