#include "XML_utilities.hpp"
#include "utils.h"

#include "models/ActionNodeModel.hpp"
#include "models/DecoratorNodeModel.hpp"
#include "models/SubtreeNodeModel.hpp"

#include <QtDebug>
#include <QLineEdit>

using namespace tinyxml2;
using namespace QtNodes;

void ParseBehaviorTreeXML(const XMLElement* bt_root, QtNodes::FlowScene* scene, Node& qt_root )
{

  int nested_nodes = 0;
  QPointF cursor(0,0);
  double x_offset = 0;

  if( strcmp( bt_root->Name(), "BehaviorTree" ) != 0)
  {
    throw std::runtime_error( "expecting a node called <BehaviorTree>");
  }

  std::function<void(const XMLElement*, Node&)> recursiveStep;

  recursiveStep = [&recursiveStep, &scene, &cursor, &nested_nodes, &x_offset]
      (const XMLElement* xml_node, Node& parent_qtnode)
  {
    // TODO: attributes

    // The nodes with a ID used that QString to insert into the registry()
    QString modelID = xml_node->Name();
    if( xml_node->Attribute("ID") )
    {
      modelID = xml_node->Attribute("ID");
    }

    std::unique_ptr<NodeDataModel> dataModel = scene->registry().create( modelID );
    BehaviorTreeNodeModel* bt_node = dynamic_cast<BehaviorTreeNodeModel*>( dataModel.get() );

    if( xml_node->Attribute("name") )
    {
      if( bt_node )
      {
        bt_node->setInstanceName( xml_node->Attribute("name") );
      }
    }

    if( bt_node )
    {
      for( const XMLAttribute* attribute= xml_node->FirstAttribute();
           attribute != nullptr;
           attribute = attribute->Next() )
      {
        const QString attr_name( attribute->Name() );
        if( attr_name!= "ID" && attr_name != "name")
        {
          bt_node->setParameterValue( attr_name, attribute->Value() );
        }
      }
    }

    if (!dataModel){
      char buffer[250];
      sprintf(buffer, "No registered model with name: [%s](%s)",
              xml_node->Name(),
              modelID.toStdString().c_str() );
      throw std::logic_error( buffer );
    }

    cursor.setY( cursor.y() + 65);
    cursor.setX( nested_nodes * 400 + x_offset );

    Node& new_node = scene->createNode( std::move(dataModel), cursor);
    scene->createConnection(new_node, 0, parent_qtnode, 0 );

    nested_nodes++;

    for (const XMLElement*  child = xml_node->FirstChildElement( )  ;
         child != nullptr;
         child = child->NextSiblingElement( ) )
    {
      recursiveStep( child, new_node );
      x_offset += 30;
    }

    nested_nodes--;
    return;
  };

  // start recursion
  recursiveStep( bt_root->FirstChildElement(), qt_root );

}
//------------------------------------------------------------------

ParameterWidgetCreator buildWidgetCreator(const QString& label,ParamType type, const QString& combo_options)
{
  ParameterWidgetCreator creator;
  creator.label = label;

  if( type == ParamType::TEXT)
  {
    creator.instance_factory = []()
    {
      QLineEdit* line = new QLineEdit();
      line->setAlignment( Qt::AlignHCenter);
      line->setMaximumWidth(150);
      return line;
    };
  }
  else if( type == ParamType::INT)
  {
    creator.instance_factory = []()
    {
      QLineEdit* line = new QLineEdit();
      line->setValidator( new QIntValidator( line ));
      line->setAlignment( Qt::AlignHCenter);
      line->setMaximumWidth(80);
      return line;
    };
  }
  else if( type == ParamType::DOUBLE)
  {
    creator.instance_factory = []()
    {
      QLineEdit* line = new QLineEdit();
      line->setValidator( new QDoubleValidator( line ));
      line->setAlignment( Qt::AlignHCenter);
      line->setMaximumWidth(120);
      return line;
    };
  }
  else if( type == ParamType::COMBO)
  {
    QStringList option_list = combo_options.split(";", QString::SkipEmptyParts);
    creator.instance_factory = [option_list]()
    {
      QComboBox* combo = new QComboBox();
      combo->addItems(option_list);
      combo->setMaximumWidth(150);
      return combo;
    };
  }
  return creator;
}

static
void buildTreeNodeModel(const tinyxml2::XMLElement* node,
                        QtNodes::DataModelRegistry& registry,
                        TreeNodeModels& models_list,
                        bool is_tree_node_model)
{

  TreeNodeModel node_model;

  QString node_name (node->Name());
  QString ID = node_name;
  if(  node->Attribute("ID") )
  {
    ID = QString(node->Attribute("ID"));
  }

  if( registry.registeredModelCreators().count(ID) > 0)
  {
    return;
  }

  const auto node_type = getNodeTypeFromString(node_name);
  node_model.node_type = node_type;

  ParameterWidgetCreators parameters;

  if( is_tree_node_model)
  {
    for (const XMLElement* param_node = node->FirstChildElement("Parameter");
         param_node != nullptr;
         param_node = param_node->NextSiblingElement("Parameter") )
    {
      const auto param_type = getParamTypeFromString( param_node->Attribute("type"));
      const auto param_name = param_node->Attribute("label");

      auto widget_creator = buildWidgetCreator( param_name, param_type,
                                                param_node->Attribute("options") );
      parameters.push_back(widget_creator);
      node_model.params.insert( std::make_pair(param_name, param_type) );
    }
  }
  else
  {
    for (const XMLAttribute* attr = node->FirstAttribute();
         attr != nullptr;
         attr = attr->Next() )
    {
      QString attr_name( attr->Name() );
      if(attr_name != "ID" && attr_name != "name")
      {
        const auto& param_type = ParamType::TEXT;
        const auto  param_name = attr_name;

        auto widget_creator = buildWidgetCreator( param_name, param_type, QString() );
        parameters.push_back(widget_creator);
        node_model.params.insert( std::make_pair(param_name, param_type) );
      }
    }
  }

  if( node_type == NodeType::ACTION )
  {
    DataModelRegistry::RegistryItemCreator node_creator = [ID, parameters]()
    {
      return std::unique_ptr<ActionNodeModel>( new ActionNodeModel(ID, parameters) );
    };
    registry.registerModel("Action", node_creator);
  }
  else if( node_type == NodeType::DECORATOR )
  {
    DataModelRegistry::RegistryItemCreator node_creator = [ID, parameters]()
    {
      return std::unique_ptr<DecoratorNodeModel>( new DecoratorNodeModel(ID, parameters) );
    };
    registry.registerModel("Decorator", node_creator);
  }
  else if( node_type == NodeType::SUBTREE )
  {
    DataModelRegistry::RegistryItemCreator node_creator = [ID, parameters]()
    {
      return std::unique_ptr<SubtreeNodeModel>( new SubtreeNodeModel(ID,parameters) );
    };
    registry.registerModel("SubTree", node_creator);
  }

  if( node_type != NodeType::UNDEFINED)
  {
    models_list.insert( std::make_pair(ID, node_model) );
  }

  qDebug() << "registered " << ID;
}

//------------------------------------------------------------------

void ReadTreeNodesModel(const tinyxml2::XMLElement* root,
                        QtNodes::DataModelRegistry& registry,
                        TreeNodeModels& models_list)
{
  using QtNodes::DataModelRegistry;

  auto model_root = root->FirstChildElement("TreeNodesModel");

  if( model_root )
  {
    for( const XMLElement* node = model_root->FirstChildElement();
         node != nullptr;
         node = node->NextSiblingElement() )
    {
      buildTreeNodeModel(node, registry, models_list, true);
    }
  }

  std::function<void(const XMLElement*)> recursiveStep;
  recursiveStep = [&](const XMLElement* node)
  {
    buildTreeNodeModel(node, registry, models_list, false);

    for( const XMLElement* child = node->FirstChildElement();
         child != nullptr;
         child = child->NextSiblingElement() )
    {
      recursiveStep(child);
    }
  };

  for( const XMLElement* bt_root = root->FirstChildElement("BehaviorTree");
       bt_root != nullptr;
       bt_root = bt_root->NextSiblingElement("BehaviorTree") )
  {
    recursiveStep( bt_root->FirstChildElement() );
  }
}



void RecursivelyCreateXml(const FlowScene &scene, XMLDocument &doc, XMLElement *parent_element, const Node *node)
{
  using namespace tinyxml2;
  const QtNodes::NodeDataModel* node_model = node->nodeDataModel();
  const std::string model_name = node_model->name().toStdString();

  const auto* bt_node = dynamic_cast<const BehaviorTreeNodeModel*>(node_model);

  XMLElement* element = doc.NewElement( bt_node->className() );

  if( !bt_node ) return;

  if( dynamic_cast<const ActionNodeModel*>(node_model) ||
      dynamic_cast<const DecoratorNodeModel*>(node_model) ||
      dynamic_cast<const SubtreeNodeModel*>(node_model) )
  {
    element->SetAttribute("ID", bt_node->registrationName().toStdString().c_str() );
  }

  auto parameters = bt_node->getCurrentParameters();
  for(const auto& param: parameters)
  {
    element->SetAttribute( param.first.toStdString().c_str(),
                           param.second.toStdString().c_str() );
  }

  if( bt_node->instanceName() != bt_node->registrationName())
  {
    element->SetAttribute("name", bt_node->instanceName().toStdString().c_str() );
  }
  parent_element->InsertEndChild( element );

  auto node_children = getChildren(scene, *node );
  for(const QtNodes::Node* child : node_children)
  {
    RecursivelyCreateXml(scene, doc, element, child );
  }
}
