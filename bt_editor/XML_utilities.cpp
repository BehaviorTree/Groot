#include "XML_utilities.hpp"
#include "utils.h"

#include "models/SubtreeNodeModel.hpp"
#include <behaviortree_cpp_v3/xml_parsing.h>
#include <QMessageBox>
#include <QtDebug>
#include <QLineEdit>

using namespace QtNodes;

NodeModel buildTreeNodeModelFromXML(const QDomElement& node)
{
    PortModels ports_list;

    QString tag_name = node.tagName();
    QString ID = tag_name;
    if(  node.hasAttribute("ID") )
    {
        ID = QString(node.attribute("ID"));
    }

    const auto node_type = BT::convertFromString<BT::NodeType>(tag_name.toStdString());

    if( node_type == BT::NodeType::UNDEFINED )
    {
        return {};
    }

    // this make sense for ports inside the <BehaviorTree> tag
    QDomNamedNodeMap attributes = node.attributes ();
    for (int i=0; i< attributes.size(); i++ )
    {
        QDomAttr attr = attributes.item(i).toAttr();

        QString attr_name( attr.name() );
        if(attr_name != "ID" && attr_name != "name")
        {
            PortModel port_model;
            port_model.direction = PortDirection::INOUT;
            ports_list.insert( { attr_name, std::move(port_model)} );
        }
    }
    // this is used for ports inside the <TreeNodesModel> tag
    const std::vector<std::pair<QString, PortDirection>> portsTypes = {
        {"input_port", PortDirection::INPUT},
        {"output_port", PortDirection::OUTPUT},
        {"inout_port", PortDirection::INOUT}};

    for(const auto& it: portsTypes)
    {
        for( QDomElement port_element = node.firstChildElement( it.first );
             !port_element.isNull();
             port_element = port_element.nextSiblingElement( it.first ) )
        {
            PortModel port_model;
            port_model.direction = it.second;
            port_model.description = port_element.text();

            if( port_element.hasAttribute("type") )
            {
                port_model.type_name = port_element.attribute("type");
            }
            if( port_element.hasAttribute("default") )
            {
                port_model.default_value = port_element.attribute("default");
            }

            if( port_element.hasAttribute("name") )
            {
                auto attr_name = port_element.attribute("name");
                ports_list.insert( { attr_name, std::move(port_model)} );
            }
        }
    }

    return { node_type, ID, ports_list };
}

//------------------------------------------------------------------

NodeModels ReadTreeNodesModel(const QDomElement &root)
{
    NodeModels models;
    using QtNodes::DataModelRegistry;

    auto model_root = root.firstChildElement("TreeNodesModel");

    if( !model_root.isNull() )
    {
        for( QDomElement node = model_root.firstChildElement();
             !node.isNull();
             node = node.nextSiblingElement() )
        {
            auto model = buildTreeNodeModelFromXML(node);
            models.insert( {model.registration_ID, model} );
        }
    }

    std::function<void(QDomElement)> recursiveStep;
    recursiveStep = [&](QDomElement node)
    {
        auto model = buildTreeNodeModelFromXML(node);
        if( model.type != NodeType::UNDEFINED &&
            model.registration_ID.isEmpty() == false &&
            models.count(model.registration_ID) == 0)
        {
            models.insert( {model.registration_ID, model} );
        }

        for( QDomElement child = node.firstChildElement();
             !child.isNull();
             child = child.nextSiblingElement() )
        {
            recursiveStep(child);
        }
    };

    for( QDomElement bt_root = root.firstChildElement("BehaviorTree");
         !bt_root.isNull();
         bt_root = bt_root.nextSiblingElement("BehaviorTree") )
    {
        auto first_child = bt_root.firstChildElement();
        if( first_child.tagName() == "Root")
        {
            first_child = first_child.firstChildElement();
        }
        recursiveStep( bt_root.firstChildElement() );
    }
    return models;
}



void RecursivelyCreateXml(const FlowScene &scene, QDomDocument &doc, QDomElement& parent_element, const Node *node)
{

    const QtNodes::NodeDataModel* node_model = node->nodeDataModel();
    const std::string model_name = node_model->name().toStdString();

    const auto* bt_node = dynamic_cast<const BehaviorTreeDataModel*>(node_model);

    QString registration_name = bt_node->registrationName();
    QDomElement element;

    if( BuiltinNodeModels().count(registration_name) != 0)
    {
        element = doc.createElement( registration_name.toStdString().c_str() );
    }
    else{
        element = doc.createElement( QString::fromStdString(toStr(bt_node->nodeType())) );
        element.setAttribute("ID", registration_name.toStdString().c_str() );
    }


    bool is_subtree_expanded = false;
    if( auto subtree = dynamic_cast<const SubtreeNodeModel*>(node_model)  )
    {
        is_subtree_expanded = subtree->expanded();
    }

    if( bt_node->instanceName() != registration_name )
    {
        element.setAttribute("name", bt_node->instanceName().toStdString().c_str() );
    }

    auto port_mapping = bt_node->getCurrentPortMapping();
    for(const auto& port_it: port_mapping)
    {
        element.setAttribute( port_it.first, port_it.second );
    }

    parent_element.appendChild( element );

    if( !is_subtree_expanded )
    {
        auto node_children = getChildren(scene, *node, true );
        for(const QtNodes::Node* child : node_children)
        {
            RecursivelyCreateXml(scene, doc, element, child );
        }
    }
}

bool VerifyXML(QDomDocument &doc,
               const std::vector<QString>& registered_ID,
               std::vector<QString>& error_messages)
{
    error_messages.clear();
    try {
        std::string xml_text = doc.toString().toStdString();
        std::set<std::string> registered_nodes;

        for(const auto& str: registered_ID)
        {
            registered_nodes.insert( str.toStdString() );
        }

        BT::VerifyXML(xml_text, registered_nodes); // may throw
    } catch (std::exception& ex)
    {
        error_messages.push_back(ex.what());
    }
    return true;
}



QDomElement writePortModel(const QString& port_name, const PortModel& port, QDomDocument& doc)
{
  QDomElement port_element;
  switch (port.direction)
  {
    case PortDirection::INPUT:
      port_element = doc.createElement("input_port");
      break;
    case PortDirection::OUTPUT:
      port_element = doc.createElement("output_port");
      break;
    case PortDirection::INOUT:
      port_element = doc.createElement("inout_port");
      break;
  }

  port_element.setAttribute("name", port_name);
  if (port.type_name.isEmpty() == false)
  {
    port_element.setAttribute("type", port.type_name);
  }
  if (port.default_value.isEmpty() == false)
  {
    port_element.setAttribute("default", port.default_value);
  }

  if (!port.description.isEmpty())
  {
    QDomText description = doc.createTextNode(port.description);
    port_element.appendChild(description);
  }
  return port_element;
}
