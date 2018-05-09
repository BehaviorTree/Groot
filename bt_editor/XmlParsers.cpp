#include "XmlParsers.hpp"
#include "utils.h"
#include <nodes/NodeStyle>
#include <functional>

void ParseBehaviorTreeXML(const QDomElement &xml_root, QtNodes::FlowScene* scene, QtNodes::Node& qt_root )
{
    using namespace QtNodes;

    int nested_nodes = 0;
    QPointF cursor(0,0);

    if( xml_root.nodeName() != "BehaviorTree")
    {
        throw std::runtime_error( "expecting a node called <BehaviorTree>");
    }

    if( xml_root.childNodes().size() != 1)
    {
        throw std::runtime_error( "<BehaviorTree> must have a single child");
    }

    std::function<void(const QDomNode&, Node&)> recursiveStep;

    recursiveStep = [&recursiveStep, &scene, &cursor, &nested_nodes](const QDomNode& xml_node, Node& parent_qtnode)
    {
        QDomNamedNodeMap attributes = xml_node.attributes();

        QJsonObject node_json;
        for(int index = 0 ; index < attributes.length(); index++)
        {
            auto item = attributes.item(index);
            node_json[item.nodeName()] = item.nodeValue();
        }

        QString ID = xml_node.nodeName();
        if( xml_node.attributes().contains("ID") )
        {
            ID = xml_node.attributes().namedItem("ID").nodeValue();
        }

        std::unique_ptr<NodeDataModel> dataModel = scene->registry().create( ID );

        if (!dataModel){
            char buffer[250];
            sprintf(buffer, "No registered model with name: [%s](%s) ",
                    xml_node.nodeName().toStdString().c_str(),
                    ID.toStdString().c_str() );
            throw std::logic_error( buffer );
        }
        dataModel->restore(node_json);

        Node& new_node = scene->createNode( std::move(dataModel) );

        printf("create new node %s: in %d out %d \n",ID.toStdString().c_str(),
               new_node.nodeDataModel()->nPorts(PortType::In),
               new_node.nodeDataModel()->nPorts(PortType::Out) );
        std::cout << std::endl;

        cursor.setY( cursor.y() + 65);
        cursor.setX( nested_nodes * 400 );

        scene->setNodePosition(new_node, cursor);
        scene->createConnection(new_node, 0, parent_qtnode, 0 );

        nested_nodes++;

        for (QDomElement  child = xml_node.firstChildElement( )  ;
             child.isNull() == false;
             child = child.nextSiblingElement( ) )
        {
            recursiveStep( child, new_node );
        }

        nested_nodes--;
        return;
    };

    // start recursion
    recursiveStep( xml_root.firstChild(), qt_root );

}

//--------------------------------------

bool StateUpdateXmlHandler::startElement(const QString &, const QString &, const QString &qName, const QXmlAttributes &atts)
{
    if( qName == "savedTreeState")
    {
        _nested_nodes.clear();
        _nested_nodes.push_back( {_root, 0} );
        return true;
    }

    QString name  = atts.value("name");
    QString state = atts.value("state");

    auto parent_node = _nested_nodes.back().first;
    int& child_index = _nested_nodes.back().second;

    std::vector<QtNodes::Node*> children_nodes =  getChildren( *_scene, *parent_node);

    if( child_index >= children_nodes.size())
    {
        return false;
    }

    QtNodes::Node* child_node = children_nodes.at( child_index );
    QtNodes::NodeStyle style = child_node->nodeDataModel()->nodeStyle();

    if( state == "SUCCESS")
    {
        style.NormalBoundaryColor = QColor(102, 255, 51);
        style.GradientColor0
                = style.GradientColor1
                = style.GradientColor2
                = style.GradientColor3 = QColor(51, 153, 51);
    }
    else if( state == "RUNNING")
    {
        style.NormalBoundaryColor = QColor(255, 204, 0);
        style.GradientColor0
                = style.GradientColor1
                = style.GradientColor2
                = style.GradientColor3 = QColor(204, 122, 0);
    }
    else if( state == "FAILURE")
    {
        style.NormalBoundaryColor = QColor(255, 51, 0);
        style.GradientColor0
                = style.GradientColor1
                = style.GradientColor2
                = style.GradientColor3 = QColor(204, 51, 0);
    }
    else if( state == "IDLE")
    {
        auto previous_coundary = child_node->nodeDataModel()->nodeStyle().NormalBoundaryColor;
        style.NormalBoundaryColor = previous_coundary;
    }

    child_node->nodeDataModel()->setNodeStyle( style );

    child_index++;
    _nested_nodes.push_back( { child_node, 0 } );
    return true;
}


bool StateUpdateXmlHandler::endElement(const QString &, const QString &, const QString &)
{
    _nested_nodes.pop_back();
    return true;
}


StateUpdateXmlHandler::StateUpdateXmlHandler(QtNodes::FlowScene *scene, QtNodes::Node *root):
    QXmlDefaultHandler(),  _scene(scene), _root(root)
{

}
