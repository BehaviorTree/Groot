#include "utils.h"
#include <nodes/Node>
#include <QDebug>
#include <set>

std::vector<QtNodes::Node*> findRoots(const QtNodes::FlowScene &scene)
{
    std::set<QUuid> roots;
    for (auto& it: scene.nodes() ){
        roots.insert( it.first );
    }

    for (auto it: scene.connections())
    {
        std::shared_ptr<QtNodes::Connection> connection = it.second;
        QtNodes::Node* child  = connection->getNode( QtNodes::PortType::In );

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
        return scene.getNodePosition( *a ).y() <  scene.getNodePosition( *b ).y();
    };
    std::sort(children.begin(), children.end(), CompareFunction );

    return children;
}


//---------------------------------------------------
void NodeReorderRecursive(QtNodes::FlowScene &scene,
                             QtNodes::Node& node,
                             QPointF cursor,
                             int level,
                             std::map<int, std::vector<QtNodes::Node*>>& nodes_by_level)
{
    const double vertical_spacing = 15;
    std::vector<QtNodes::Node*> children = getChildren(scene, node );

    double total_height = 0;
    for (QtNodes::Node* child_node: children)
    {
        total_height += child_node->nodeGeometry().height() + vertical_spacing;
    }

    auto this_level_it = nodes_by_level.find(level);
    auto next_level_it = nodes_by_level.find(level+1);

    double this_max_Y = 0;
    double next_max_Y = cursor.y();

    if( next_level_it != nodes_by_level.end() )
    {
        QtNodes::Node& last_node_right = *(next_level_it->second.back()) ;
        next_max_Y = vertical_spacing*3.0 +
                scene.getNodePosition( last_node_right ).y() +
                scene.getNodeSize( last_node_right ).height();
    }

    if( this_level_it != nodes_by_level.end() )
    {
        QtNodes::Node& last_node_right = *(this_level_it->second.back()) ;
        this_max_Y = vertical_spacing*2.0 +
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

    QPointF children_cursor( cursor.x() + node.nodeGeometry().width() + 100, cursor.y() ) ;

    if( children.size() > 1){
        children_cursor.setY( cursor.y() - total_height*0.5 );
    }

    for (int i=0; i< children.size(); i++)
    {
        QtNodes::Node* child_node = children[i];
        const double height = child_node->nodeGeometry().height();
        NodeReorderRecursive( scene, *child_node, children_cursor, level+1, nodes_by_level  );
        double child_y   =  children_cursor.y() + height + 2.0*vertical_spacing;
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
}
