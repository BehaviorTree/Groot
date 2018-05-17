#include "utils.h"
#include <nodes/Node>
#include <QDebug>
#include <set>

#include "models/ActionNodeModel.hpp"
#include "models/DecoratorNodeModel.hpp"
#include "models/ControlNodeModel.hpp"
#include "models/SubtreeNodeModel.hpp"
#include "models/RootNodeModel.hpp"

using QtNodes::PortLayout;

std::vector<QtNodes::Node*> findRoots(const QtNodes::FlowScene &scene)
{
  std::set<QUuid> roots;
  for (auto& it: scene.nodes() ){
    roots.insert( it.first );
  }

  for (auto it: scene.connections())
  {
    std::shared_ptr<QtNodes::Connection> connection = it.second;
    const QtNodes::Node* child  = connection->getNode( QtNodes::PortType::In );

    if( child ) {
      roots.erase( child->id() );
    }
  }

  std::vector<QtNodes::Node*> sorted_roots;
  for (auto uuid: roots)
  {
    sorted_roots.push_back( scene.nodes().find(uuid)->second.get() );
  }

  auto CompareFunction = [&](const QtNodes::Node* a, const QtNodes::Node* b)
  {
    return scene.getNodePosition( *a ).y() <  scene.getNodePosition( *b ).y();
  };
  std::sort(sorted_roots.begin(), sorted_roots.end(), CompareFunction );

  return sorted_roots;
}

std::vector<QtNodes::Node*> getChildren(const QtNodes::FlowScene &scene,
                                        const QtNodes::Node& parent_node)
{
  std::vector<QtNodes::Node*> children;

  for (auto it: scene.connections())
  {
    std::shared_ptr<QtNodes::Connection> connection = it.second;

    QtNodes::Node* parent = connection->getNode( QtNodes::PortType::Out );
    QtNodes::Node* child  = connection->getNode( QtNodes::PortType::In );

    if( parent && child )
    {
      if( parent->id() == parent_node.id())
      {
        children.push_back( child );
      }
    }
  }

  if( scene.layout() == PortLayout::Vertical)
  {
    std::sort(children.begin(), children.end(),
              [&](const QtNodes::Node* a, const QtNodes::Node* b)
    {
      return scene.getNodePosition( *a ).x() <  scene.getNodePosition( *b ).x();
    } );
  }
  else{
    std::sort(children.begin(), children.end(),
              [&](const QtNodes::Node* a, const QtNodes::Node* b)
    {
      return scene.getNodePosition( *a ).y() <  scene.getNodePosition( *b ).y();
    } );
  }

  return children;
}


//---------------------------------------------------

void RecursiveNodeReorder(AbstractGeometry* root_node, PortLayout layout)
{
  std::function<void(unsigned, AbstractGeometry* node)> recursiveStep;

  std::vector<QPointF> layer_cursor;
  std::vector< std::vector<AbstractGeometry*> > nodes_by_level(1);
  layer_cursor.push_back(QPointF(0,0));
  const qreal LEVEL_SPACING = 100;
  const qreal NODE_SPACING  = 40;

  recursiveStep = [&](unsigned current_layer, AbstractGeometry* node)
  {
    node->pos = layer_cursor[current_layer];
    nodes_by_level[current_layer].push_back( node );

    //------------------------------------
    if( node->children.size() > 0 )
    {
      qreal recommended_pos = NODE_SPACING * 0.5;

      current_layer++;

      if( layout == PortLayout::Vertical)
      {
        recommended_pos += (node->pos.x() + node->size.width()*0.5)  ;
        for(auto& child: node->children)
        {
          recommended_pos -=  (child->size.width() + NODE_SPACING) * 0.5;
        }

        if( current_layer >= layer_cursor.size())
        {
          QPointF new_cursor( recommended_pos,
                              node->pos.y() + LEVEL_SPACING );
          layer_cursor.push_back( new_cursor );
          nodes_by_level.push_back( std::vector<AbstractGeometry*>() );
        }
        else{
          recommended_pos = std::max( recommended_pos, layer_cursor[current_layer].x() );
          layer_cursor[current_layer].setX(recommended_pos);
        }
      }//------------------------------------
      else{
        recommended_pos += node->pos.y() + node->size.height()*0.5;
        for(auto& child: node->children)
        {
          recommended_pos -=  (child->size.height() + NODE_SPACING) * 0.5;
        }

        if( current_layer >= layer_cursor.size())
        {
          QPointF new_cursor( node->pos.x() + LEVEL_SPACING,
                              recommended_pos);
          layer_cursor.push_back( new_cursor );
          nodes_by_level.push_back( std::vector<AbstractGeometry*>() );
        }
        else{
          recommended_pos = std::max( recommended_pos, layer_cursor[current_layer].y() );
          layer_cursor[current_layer].setY(recommended_pos);
        }
      }
      //------------------------------------

      auto initial_pos = layer_cursor[current_layer];

      for(auto& child: node->children)
      {
        recursiveStep(current_layer, child);
        if( layout == PortLayout::Vertical)
        {
          layer_cursor[current_layer]+= QPointF( child->size.width() + NODE_SPACING, 0 );
        }
        else{
          layer_cursor[current_layer]+= QPointF( 0, child->size.height() + NODE_SPACING );
        }
      }

      auto final_pos = layer_cursor[current_layer];

      // rebalance father
      if( layout == PortLayout::Vertical)
      {
        double diff = (node->pos.x() + node->size.width()*0.5) - (final_pos.x() + initial_pos.x() - NODE_SPACING)* 0.5 ;
        node->pos += QPointF( - diff, 0 );
        layer_cursor[current_layer -1 ] += QPointF( - diff, 0 );
      }
      else{
        double diff = (node->pos.y() + node->size.height()*0.5) - (final_pos.y() + initial_pos.y() - NODE_SPACING)* 0.5 ;
        node->pos += QPointF( 0, - diff );
        layer_cursor[current_layer -1 ] += QPointF( 0, - diff );
      }
    }
  };

  recursiveStep(0, root_node);

  if( layout == PortLayout::Vertical)
  {
    qreal offset = root_node->size.height() + LEVEL_SPACING;
    for(unsigned i=1; i< nodes_by_level.size(); i++ )
    {
      qreal max_height = 0;
      for(auto node: nodes_by_level[i])
      {
        max_height = std::max( max_height, node->size.height() );
        node->pos.setY( offset );
      }
      offset += max_height + LEVEL_SPACING;
    }
  }
  else{
    qreal offset = root_node->size.width() + LEVEL_SPACING;
    for(unsigned i=1; i< nodes_by_level.size(); i++ )
    {
      qreal max_width = 0;
      for(auto node: nodes_by_level[i])
      {
        max_width = std::max( max_width, node->size.width() );
        node->pos.setX( offset );
      }
      offset += max_width + LEVEL_SPACING;
    }
  }

}


AbstractTree BuildAbstractTree(QtNodes::FlowScene &scene)
{
  std::map<QtNodes::Node*, AbstractGeometry> abstract_nodes;

  for (const auto& it: scene.nodes() )
  {
    QtNodes::Node* node = (it.second.get());
    AbstractGeometry abs_node;

    abs_node.name = node->nodeDataModel()->name();
    abs_node.pos  = scene.getNodePosition(*node) ;
    abs_node.size = scene.getNodeSize(*node);
    abstract_nodes.insert( std::make_pair( node, abs_node) );
  }

  for (auto& it: abstract_nodes)
  {
    const QtNodes::Node& node = *(it.first);
    AbstractGeometry& abs_node = it.second;

    auto children = getChildren(scene, node );
    abs_node.children.reserve( children.size() );

    for(auto& child: children )
    {
      abs_node.children.push_back( &( abstract_nodes[child] ) );
    }
  }
  return abstract_nodes;
}

void NodeReorder(QtNodes::FlowScene &scene, AbstractTree abstract_tree)
{
  auto roots = findRoots(scene);
  if( roots.size() != 1)
  {
    return;
  }

  RecursiveNodeReorder( &abstract_tree[ roots.front() ], scene.layout() );

  for (auto& it: abstract_tree)
  {
    QtNodes::Node& node = *(it.first);
    scene.setNodePosition( node, it.second.pos );
  }
}

QString getCategory(const QtNodes::NodeDataModel *dataModel)
{
  QString category;
  if( dynamic_cast<const ActionNodeModel*>(dataModel) )
  {
    category = "Action";
  }
  else if( dynamic_cast<const DecoratorNodeModel*>(dataModel) )
  {
    category = "Decorator";
  }
  else if( dynamic_cast<const ControlNodeModel*>(dataModel) )
  {
    category = "Control";
  }
  else if( dynamic_cast<const RootNodeModel*>(dataModel) )
  {
    category = "Root";
  }
  else if( dynamic_cast<const SubtreeNodeModel*>(dataModel) )
  {
    category = "SubTree";
  }
  return category;
}


