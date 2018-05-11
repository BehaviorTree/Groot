#include "XmlParsers.hpp"
#include "utils.h"

#include "models/ActionNodeModel.hpp"
#include "models/DecoratorNodeModel.hpp"
#include "models/SubtreeNodeModel.hpp"

using namespace tinyxml2;
using namespace QtNodes;

void ParseBehaviorTreeXML(const XMLElement* bt_root, QtNodes::FlowScene* scene, Node& qt_root )
{

  int nested_nodes = 0;
  QPointF cursor(0,0);

  if( strcmp( bt_root->Name(), "BehaviorTree" ) != 0)
  {
    throw std::runtime_error( "expecting a node called <BehaviorTree>");
  }

  std::function<void(const XMLElement*, Node&)> recursiveStep;

  recursiveStep = [&recursiveStep, &scene, &cursor, &nested_nodes](const XMLElement* xml_node, Node& parent_qtnode)
  {
    // TODO: attributes

    // The nodes with a ID used that QString to insert into the registry()
    QString modelID = xml_node->Name();
    if( xml_node->Attribute("ID") )
    {
      modelID = xml_node->Attribute("ID");
    }

    std::unique_ptr<NodeDataModel> dataModel = scene->registry().create( modelID );

    if( xml_node->Attribute("name") )
    {
      if( auto ptr = dynamic_cast<BehaviorTreeNodeModel*>( dataModel.get() ) )
      {
        ptr->setInstanceName( xml_node->Attribute("name") );
      }
    }

    if (!dataModel){
      char buffer[250];
      sprintf(buffer, "No registered model with name: [%s](%s) ",
              xml_node->Name(),
              modelID.toStdString().c_str() );
        throw std::logic_error( buffer );
    }

    cursor.setY( cursor.y() + 65);
    cursor.setX( nested_nodes * 400 );

    Node& new_node = scene->createNode( std::move(dataModel), cursor);
    scene->createConnection(new_node, 0, parent_qtnode, 0 );

    nested_nodes++;

    for (const XMLElement*  child = xml_node->FirstChildElement( )  ;
         child != nullptr;
         child = child->NextSiblingElement( ) )
    {
      recursiveStep( child, new_node );
    }

    nested_nodes--;
    return;
  };

  // start recursion
  recursiveStep( bt_root->FirstChildElement(), qt_root );

}


bool ReadTreeNodesModel(QtNodes::DataModelRegistry& registry,
                        const tinyxml2::XMLElement* model_root)
{
  using QtNodes::DataModelRegistry;

  for( const XMLElement* node = model_root->FirstChildElement();
       node != nullptr;
       node = node->NextSiblingElement() )
  {
    const char* node_name = node->Name();
    QString ID;
    if(  node->Attribute("ID") )
    {
      ID = QString(node->Attribute("ID"));
    }
    else{
      ID = QString(node_name);
    }

    const ParameterWidgetCreators parameters;

    if( ! strcmp( node_name, "Action" ) )
    {
      DataModelRegistry::RegistryItemCreator creator = [ID, parameters]()
      {
        return std::unique_ptr<ActionNodeModel>( new ActionNodeModel(ID) );
      };
      registry.registerModel("Action", creator);
    }
    else if( ! strcmp( node_name, "Decorator" ) )
    {
      DataModelRegistry::RegistryItemCreator creator = [ID, parameters]()
      {
        return std::unique_ptr<DecoratorNodeModel>( new DecoratorNodeModel(ID, parameters) );
      };
      registry.registerModel("Decorator", creator);

    }
    else if( ! strcmp( node_name, "SubTree" ) )
    {
      DataModelRegistry::RegistryItemCreator creator = [ID, parameters]()
      {
        return std::unique_ptr<SubtreeNodeModel>( new SubtreeNodeModel(ID,parameters) );
      };
      registry.registerModel("SubTree", creator);
    }
  }
  return true;
}


