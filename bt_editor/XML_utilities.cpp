#include "XML_utilities.hpp"
#include "utils.h"

#include "models/ActionNodeModel.hpp"
#include "models/DecoratorNodeModel.hpp"
#include "models/SubtreeNodeModel.hpp"

#include <QtDebug>
#include <QLineEdit>
#include <QMessageBox>

using namespace tinyxml2;
using namespace QtNodes;


std::pair<QString,TreeNodeModel>
buildTreeNodeModel(const tinyxml2::XMLElement* node,
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
    return std::make_pair(ID, node_model );
}

//------------------------------------------------------------------

TreeNodeModels ReadTreeNodesModel(const tinyxml2::XMLElement* root)
{
    TreeNodeModels models;
    using QtNodes::DataModelRegistry;

    auto model_root = root->FirstChildElement("TreeNodesModel");

    if( model_root )
    {
        for( const XMLElement* node = model_root->FirstChildElement();
             node != nullptr;
             node = node->NextSiblingElement() )
        {
            auto model_pair = buildTreeNodeModel(node, true);
            models.insert( model_pair );
        }
    }

    std::function<void(const XMLElement*)> recursiveStep;
    recursiveStep = [&](const XMLElement* node)
    {
        auto model_pair = buildTreeNodeModel(node, true);
        models.insert( model_pair );

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
    return models;
}



void RecursivelyCreateXml(const FlowScene &scene, XMLDocument &doc, XMLElement *parent_element, const Node *node)
{
    using namespace tinyxml2;
    const QtNodes::NodeDataModel* node_model = node->nodeDataModel();
    const std::string model_name = node_model->name().toStdString();

    const auto* bt_node = dynamic_cast<const BehaviorTreeDataModel*>(node_model);

    XMLElement* element = doc.NewElement( bt_node->className() );

    if( !bt_node ) return;

    QString registration_name = bt_node->registrationName();

    bool is_subtree_expanded = false;
    if( auto subtree = dynamic_cast<const SubtreeNodeModel*>(node_model)  )
    {
        is_subtree_expanded = subtree->expanded();
    }

    element->SetAttribute("ID", registration_name.toStdString().c_str() );
    if( bt_node->instanceName() != registration_name )
    {
        element->SetAttribute("name", bt_node->instanceName().toStdString().c_str() );
    }

    auto parameters = bt_node->getCurrentParameters();
    for(const auto& param: parameters)
    {
        element->SetAttribute( param.first.toStdString().c_str(),
                               param.second.toStdString().c_str() );
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

bool VerifyXML(XMLDocument &doc,
               const std::vector<QString>& registered_ID,
               std::vector<QString>& error_messages)
{
    if (doc.Error())
    {
        error_messages.emplace_back("The XML was not correctly loaded");
        return false;
    }
    bool is_valid = true;

    //-------- Helper functions (lambdas) -----------------
    auto strEqual = [](const char* str1, const char* str2) -> bool {
        return strcmp(str1, str2) == 0;
    };

    auto AppendError = [&](int line_num, const char* text) {
        char buffer[256];
        sprintf(buffer, "Error at line %d: -> %s", line_num, text);
        error_messages.emplace_back(buffer);
        is_valid = false;
    };

    auto ChildrenCount = [](const tinyxml2::XMLElement* parent_node) {
        int count = 0;
        for (auto node = parent_node->FirstChildElement(); node != nullptr;
             node = node->NextSiblingElement())
        {
            count++;
        }
        return count;
    };

    //-----------------------------

    const tinyxml2::XMLElement* xml_root = doc.RootElement();

    if (!xml_root || !strEqual(xml_root->Name(), "root"))
    {
        error_messages.emplace_back("The XML must have a root node called <root>");
        return false;
    }
    //-------------------------------------------------
    auto meta_root = xml_root->FirstChildElement("TreeNodesModel");
    auto meta_sibling = meta_root ? meta_root->NextSiblingElement("TreeNodesModel") : nullptr;

    if (meta_sibling)
    {
        AppendError(meta_sibling->GetLineNum(), " Only a single node <TreeNodesModel> is supported");
    }
    if (meta_root)
    {
        // not having a MetaModel is not an error. But consider that the
        // Graphical editor needs it.
        for (auto node = xml_root->FirstChildElement(); node != nullptr;
             node = node->NextSiblingElement())
        {
            const char* name = node->Name();
            if (strEqual(name, "Action") || strEqual(name, "Decorator") ||
                    strEqual(name, "SubTree") || strEqual(name, "Condition"))
            {
                const char* ID = node->Attribute("ID");
                if (!ID)
                {
                    AppendError(node->GetLineNum(), "Error at line %d: -> The attribute [ID] is "
                                                    "mandatory");
                }
                for (auto param_node = xml_root->FirstChildElement("Parameter");
                     param_node != nullptr;
                     param_node = param_node->NextSiblingElement("Parameter"))
                {
                    const char* label = node->Attribute("label");
                    const char* type = node->Attribute("type");
                    if (!label || !type)
                    {
                        AppendError(node->GetLineNum(),
                                    "The node <Parameter> requires the attributes [type] and "
                                    "[label]");
                    }
                }
            }
        }
    }

    //-------------------------------------------------

    // function to be called recursively
    std::function<void(const tinyxml2::XMLElement*)> recursiveStep;

    recursiveStep = [&](const tinyxml2::XMLElement* node) {
        const int children_count = ChildrenCount(node);
        const char* name = node->Name();
        if (strEqual(name, "Decorator"))
        {
            if (children_count != 1)
            {
                AppendError(node->GetLineNum(), "The node <Decorator> must have exactly 1 child");
            }
            if (!node->Attribute("ID"))
            {
                AppendError(node->GetLineNum(), "The node <Decorator> must have the attribute "
                                                "[ID]");
            }
        }
        else if (strEqual(name, "Action"))
        {
            if (children_count != 0)
            {
                AppendError(node->GetLineNum(), "The node <Action> must not have any child");
            }
            if (!node->Attribute("ID"))
            {
                AppendError(node->GetLineNum(), "The node <Action> must have the attribute [ID]");
            }
        }
        else if (strEqual(name, "Condition"))
        {
            if (children_count != 0)
            {
                AppendError(node->GetLineNum(), "The node <Condition> must not have any child");
            }
            if (!node->Attribute("ID"))
            {
                AppendError(node->GetLineNum(), "The node <Condition> must have the attribute "
                                                "[ID]");
            }
        }
        else if (strEqual(name, "Sequence") || strEqual(name, "SequenceStar") ||
                 strEqual(name, "Fallback") || strEqual(name, "FallbackStar"))
        {
            if (children_count == 0)
            {
                AppendError(node->GetLineNum(), "A Control node must have at least 1 child");
            }
        }
        else if (strEqual(name, "SubTree"))
        {
            if (children_count > 0)
            {
                AppendError(node->GetLineNum(), "The <SubTree> node must have no children");
            }
            if (!node->Attribute("ID"))
            {
                AppendError(node->GetLineNum(), "The node <SubTree> must have the attribute [ID]");
            }
        }
        else
        {
            // Last resort:  MAYBE used ID as element name?
            bool found = false;
            for (const auto& ID : registered_ID)
            {
                if (ID == name)
                {
                    found = true;
                    break;
                }
            }
            if (!found)
            {
                AppendError(node->GetLineNum(), (std::string("Node not recognized: ") + name).c_str() );
            }
        }
        //recursion
        for (auto child = node->FirstChildElement(); child != nullptr;
             child = child->NextSiblingElement())
        {
            recursiveStep(child);
        }
    };

    std::vector<std::string> tree_names;
    int tree_count = 0;

    for (auto bt_root = xml_root->FirstChildElement("BehaviorTree"); bt_root != nullptr;
         bt_root = bt_root->NextSiblingElement("BehaviorTree"))
    {
        tree_count++;
        if (bt_root->Attribute("ID"))
        {
            tree_names.push_back(bt_root->Attribute("ID"));
        }
        if (ChildrenCount(bt_root) != 1)
        {
            AppendError(bt_root->GetLineNum(), "The node <BehaviorTree> must have exactly 1 child");
        }
        else
        {
            recursiveStep(bt_root->FirstChildElement());
        }
    }

    if (xml_root->Attribute("main_tree_to_execute"))
    {
        std::string main_tree = xml_root->Attribute("main_tree_to_execute");
        if (std::find(tree_names.begin(), tree_names.end(), main_tree) == tree_names.end())
        {
            error_messages.emplace_back("The tree specified in [main_tree_to_execute] "
                                        "can't be found");
            is_valid = false;
        }
    }
    else
    {
        if (tree_count != 1)
        {
            error_messages.emplace_back(
                        "If you don't specify the attribute [main_tree_to_execute], "
                        "Your file must contain a single BehaviorTree");
            is_valid = false;
        }
    }
    return is_valid;
}

std::set<const QString *> NotBuiltInNodes(const TreeNodeModels &models)
{
    std::set<const QString *> custom_models;

    if( models.size() > BuiltinNodeModels().size() )
    {
        for(const auto& it: models)
        {
            if( BuiltinNodeModels().count(it.first) == 0)
            {
                custom_models.insert( &it.first );
            }
        }
    }
    return custom_models;
}


