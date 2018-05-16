#include "utils.h"
#include <nodes/Node>
#include <QDebug>
#include <set>

#include "models/ActionNodeModel.hpp"
#include "models/DecoratorNodeModel.hpp"
#include "models/ControlNodeModel.hpp"
#include "models/SubtreeNodeModel.hpp"
#include "models/RootNodeModel.hpp"

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

  auto CompareFunction = [&](const QtNodes::Node* a, const QtNodes::Node* b)
  {
    return scene.getNodePosition( *a ).x() <  scene.getNodePosition( *b ).x();
  };
  std::sort(children.begin(), children.end(), CompareFunction );

  return children;
}


//---------------------------------------------------
/*
void NodeReorderRecursive(QtNodes::FlowScene &scene,
                             QtNodes::Node& node,
                             QPointF cursor,
                             int level,
                             std::map<int, std::vector<QtNodes::Node*>>& nodes_by_level)
{
    const double VERTICAL_SPACING = 30;
    const double HORIZONTAL_SPACING = 60;

    std::vector<QtNodes::Node*> children = getChildren(scene, node );

    double total_height = 0;
    for (QtNodes::Node* child_node: children)
    {
        total_height += child_node->nodeGeometry().height() + VERTICAL_SPACING;
    }

    auto this_level_it = nodes_by_level.find(level);
    auto next_level_it = nodes_by_level.find(level+1);

    double this_max_Y = 0;
    double next_max_Y = cursor.y();

    if( next_level_it != nodes_by_level.end() )
    {
        QtNodes::Node& last_node_right = *(next_level_it->second.back()) ;
        next_max_Y = VERTICAL_SPACING +
                scene.getNodePosition( last_node_right ).y() +
                scene.getNodeSize( last_node_right ).height();
    }

    if( this_level_it != nodes_by_level.end() )
    {
        QtNodes::Node& last_node_right = *(this_level_it->second.back()) ;
        this_max_Y = VERTICAL_SPACING +
                scene.getNodePosition( last_node_right ).y() +
                scene.getNodeSize( last_node_right ).height();
    }

    //---------------------------------------------
    // adjust cursor Y
    cursor.setY( std::max( this_max_Y, next_max_Y) );
//    qDebug() << "node: " << node.nodeDataModel()->caption()<< " id: "<<
//                node.nodeDataModel()->name() << " pos: " << cursor;

    scene.setNodePosition( node, cursor);
    nodes_by_level[level].push_back( &node );
    //---------------------------------------------

    QPointF children_cursor( cursor.x() + node.nodeGeometry().width() + HORIZONTAL_SPACING, cursor.y() ) ;

    if( children.size() > 1){
        children_cursor.setY( cursor.y() - total_height*0.5 );
    }

    for (unsigned i=0; i< children.size(); i++)
    {
        QtNodes::Node* child_node = children[i];
        const double height = child_node->nodeGeometry().height();
        NodeReorderRecursive( scene, *child_node, children_cursor, level+1, nodes_by_level  );
        double child_y   =  children_cursor.y() + height + 2.0*VERTICAL_SPACING;
        children_cursor.setY( child_y );
        //qDebug() << ".... cursor shifted " << prev_cursor << " to " << children_cursor;
    }

    if( children.size() > 1)
    {
        double min_Y = scene.getNodePosition( *children.front() ).y();
        double max_Y = scene.getNodePosition( *children.back() ).y();

        QPointF temp_cursor( cursor.x(),  (max_Y + min_Y) * 0.5 );
        if( temp_cursor.y() > cursor.y())
        {
            scene.setNodePosition( node, temp_cursor);
        }
    }
}


void NodeReorder(QtNodes::FlowScene &scene)
{
  //   qDebug() << "--------------------------";
    std::vector<QtNodes::Node*> roots = findRoots(scene);
    std::map<int, std::vector<QtNodes::Node*>> nodes_by_level;

    QPointF cursor(10,10);

    for (QtNodes::Node* node: roots)
    {
        NodeReorderRecursive(scene, *node, cursor, 0, nodes_by_level);
    }

    double right   = 0;
    double bottom  = 0;

    for (auto& it: scene.nodes() )
    {
        QtNodes::Node* node = it.second.get();
        node->nodeGeometry().recalculateSize();
        QPointF pos = scene.getNodePosition(*node) ;
        QSizeF rect =  scene.getNodeSize(*node);

        right  = std::max( right,  pos.x() + rect.width() );
        bottom = std::max( bottom, pos.y() + rect.height() );
    }

    scene.setSceneRect(-30, -30, right + 60, bottom + 60);
}*/

struct AbstractRectangularGeometry
{
  QString name;
  QPointF pos; // top left corner
  QSizeF size;
  std::vector<AbstractRectangularGeometry*> children;
};

void RecursiveNodeReorder(AbstractRectangularGeometry* root_node)
{
  std::function<void(int, AbstractRectangularGeometry* node)> recursiveStep;

  std::vector<QPointF> layer_cursor;
  layer_cursor.push_back(QPointF(0,0));

  recursiveStep = [&recursiveStep, &layer_cursor](int current_layer, AbstractRectangularGeometry* node)
  {
    const qreal LAYER_SPACING = 100;
    const qreal NODE_SPACING  = 40;
    node->pos = layer_cursor[current_layer];

    qDebug() << node->name << " -> " << node->pos;

    qreal max_height = 0;
    qreal total_width = -NODE_SPACING;

    for(auto& child: node->children)
    {
      max_height = std::max( max_height, child->size.height() );
      total_width +=  child->size.width() + NODE_SPACING;
    }

    if( node->children.size() > 0 )
    {
      current_layer++;
      auto recommended_X = node->pos.x() + node->size.width()*0.5 - total_width*0.5;
      if( current_layer >= layer_cursor.size())
      {
        QPointF new_cursor( recommended_X,
                            node->pos .y() + max_height + LAYER_SPACING );
        layer_cursor.push_back( new_cursor );

        qDebug() <<  current_layer << " " << new_cursor << " " << node->name;
      }
      else{
        recommended_X = std::max( recommended_X, layer_cursor[current_layer].x() );
        layer_cursor[current_layer].setX(recommended_X);
      }

      auto initial_pos = layer_cursor[current_layer];

      for(auto& child: node->children)
      {
        recursiveStep(current_layer, child);
        layer_cursor[current_layer].setX( layer_cursor[current_layer].x() +
                                          child->size.width() +
                                          NODE_SPACING  );
      }

      auto final_pos = layer_cursor[current_layer];

      // rebalance father
      auto diff = (node->pos.x() + node->size.width()*0.5) - (final_pos.x() + initial_pos.x() - NODE_SPACING)* 0.5 ;
      node->pos += QPointF( - diff, 0 );
      layer_cursor[current_layer -1 ] += QPointF( - diff, 0 );
    }
  };

  recursiveStep(0, root_node);

}

void NodeReorder(QtNodes::FlowScene &scene)
{
  auto roots = findRoots(scene);
  if( roots.size() != 1)
  {
    return;
  }

  std::map<QtNodes::Node*, AbstractRectangularGeometry> abstract_nodes;

  for (const auto& it: scene.nodes() )
  {
    QtNodes::Node* node = (it.second.get());
    AbstractRectangularGeometry abs_node;

    abs_node.name = node->nodeDataModel()->name();
    abs_node.pos  = scene.getNodePosition(*node) ;
    abs_node.size = scene.getNodeSize(*node);
    abstract_nodes.insert( std::make_pair( node, abs_node) );
  }

  for (auto& it: abstract_nodes)
  {
    const QtNodes::Node& node = *(it.first);
    AbstractRectangularGeometry& abs_node = it.second;

    auto children = getChildren(scene, node );
    abs_node.children.reserve( children.size() );

    for(auto& child: children )
    {
      abs_node.children.push_back( &( abstract_nodes[child] ) );
    }
  }

  RecursiveNodeReorder( &abstract_nodes[ roots.front() ] );

  for (auto& it: abstract_nodes)
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
