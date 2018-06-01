#include "bt_editor_base.h"


AbstractTreeNode *AbsBehaviorTree::rootNode()
{
    if( _root_node_index < 0)
        return nullptr;
    else
        return &_nodes.at( _root_node_index );
}

const AbstractTreeNode *AbsBehaviorTree::rootNode() const
{
    if( _root_node_index < 0)
        return nullptr;
    else
        return &_nodes.at( _root_node_index );
}

void AbsBehaviorTree::pushBack(uint16_t UID, AbstractTreeNode node)
{
    node.index = _nodes.size();
    _UID_to_index.insert( {UID, node.index} );
    _nodes.push_back( std::move(node) );
}

int AbsBehaviorTree::UidToIndex(uint16_t uid) const
{
    return _UID_to_index.at(uid);
}

void AbsBehaviorTree::updateRootIndex()
{
    std::vector<bool> index_has_parent(_nodes.size(), false);

    for(const auto& node: _nodes)
    {
        for(int child_index: node.children_index)
        {
            index_has_parent[ child_index ] = true;
        }
    }
    int root_count = 0;
    for(size_t index = 0; index < index_has_parent.size(); index++)
    {
        if (! index_has_parent[index] )
        {
            root_count++;
            _root_node_index = index;
        }
    }
    if( root_count != 1)
    {
        throw std::logic_error("Malformed AbsBehaviorTree");
    }
}
