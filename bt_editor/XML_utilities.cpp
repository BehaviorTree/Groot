#include "XML_utilities.hpp"
#include "utils.h"

#include "models/ActionNodeModel.hpp"
#include "models/DecoratorNodeModel.hpp"
#include "models/SubtreeNodeModel.hpp"

#include <QtDebug>
#include <QLineEdit>

using namespace tinyxml2;
using namespace QtNodes;

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

    const auto node_type = getNodeTypeFromString(node_name);
    node_model.node_type = node_type;

    ParameterWidgetCreators parameters;

    if( is_tree_node_model)
    {
        for (const XMLElement* param_node = node->FirstChildElement("Parameter");
             param_node != nullptr;
             param_node = param_node->NextSiblingElement("Parameter") )
        {
            TreeNodeModel::Param model_param;
            model_param.label = param_node->Attribute("label");

            if( param_node->Attribute("default") ){
                model_param.default_value = param_node->Attribute("default");
            }
            auto widget_creator = buildWidgetCreator( model_param );
            parameters.push_back(widget_creator);
            node_model.params.push_back( std::move(model_param) );
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
                TreeNodeModel::Param model_param;
                model_param.label = attr_name;
                model_param.default_value = attr->Value();

                auto widget_creator = buildWidgetCreator( model_param );
                parameters.push_back(widget_creator);
                node_model.params.push_back( std::move(model_param) );
            }
        }
    }

    if( node_type == NodeType::ACTION )
    {
        DataModelRegistry::RegistryItemCreator node_creator = [ID, parameters]()
        {
            return std::unique_ptr<ActionNodeModel>( new ActionNodeModel(ID, parameters) );
        };
        registry.registerModel("Action", node_creator, ID);
    }
    else if( node_type == NodeType::CONDITION )
    {
        DataModelRegistry::RegistryItemCreator node_creator = [ID, parameters]()
        {
            return std::unique_ptr<ConditionNodeModel>( new ConditionNodeModel(ID, parameters) );
        };
        registry.registerModel("Condition", node_creator, ID);
    }
    else if( node_type == NodeType::DECORATOR )
    {
        DataModelRegistry::RegistryItemCreator node_creator = [ID, parameters]()
        {
            return std::unique_ptr<DecoratorNodeModel>( new DecoratorNodeModel(ID, parameters) );
        };
        registry.registerModel("Decorator", node_creator, ID);
    }
    else if( node_type == NodeType::SUBTREE )
    {
        DataModelRegistry::RegistryItemCreator node_creator = [ID, parameters]()
        {
            return std::unique_ptr<SubtreeNodeModel>( new SubtreeNodeModel(ID,parameters) );
        };
        registry.registerModel("SubTree", node_creator, ID);

        auto otherID = ID + EXPANDED_SUFFIX;
        node_creator = [ID, otherID, parameters]()
        {
          auto node = std::unique_ptr<SubtreeExpandedNodeModel>(
                new SubtreeExpandedNodeModel(otherID, parameters) );

          node->setInstanceName(ID);
          return node;
        };
        registry.registerModel("SubTreeExpanded", node_creator, otherID);
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

    const bool is_subtree_expanded = dynamic_cast<const SubtreeExpandedNodeModel*>(node_model) != nullptr;

    const auto* bt_node = dynamic_cast<const BehaviorTreeDataModel*>(node_model);

    XMLElement* element = doc.NewElement( bt_node->className() );

    if( !bt_node ) return;

    if( is_subtree_expanded )
    {
        element->SetName( SubtreeNodeModel::Name() );
        auto registration_name = bt_node->registrationName().left( EXPANDED_SUFFIX.length() );
        element->SetAttribute("ID", registration_name.toStdString().c_str() );
    }

    else if( dynamic_cast<const ActionNodeModel*>(node_model) ||
             dynamic_cast<const ConditionNodeModel*>(node_model) ||
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

    if( !is_subtree_expanded )
    {
        auto node_children = getChildren(scene, *node, true );
        for(const QtNodes::Node* child : node_children)
        {
            RecursivelyCreateXml(scene, doc, element, child );
        }
    }
}
