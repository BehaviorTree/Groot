#include "XmlParsers.hpp"
#include "utils.h"

#include "models/ActionNodeModel.hpp"
#include "models/DecoratorNodeModel.hpp"
#include "models/SubtreeNodeModel.hpp"


bool ReadTreeNodesModel(QtNodes::DataModelRegistry& registry,
                        const tinyxml2::XMLElement* model_root)
{
  using namespace tinyxml2;
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


