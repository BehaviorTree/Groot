#include "XML_utilities.hpp"
#include "utils.h"

#include "models/ActionNodeModel.hpp"
#include "models/DecoratorNodeModel.hpp"
#include "models/SubtreeNodeModel.hpp"

#include <QtDebug>
#include <QLineEdit>


using namespace QtNodes;


TreeNodeModel
buildTreeNodeModel(const QDomElement& node)
{
    std::vector<TreeNodeModel::Param> model_params;

    QString node_name = node.nodeName();
    QString ID = node_name;
    if(  node.hasAttribute("ID") )
    {
        ID = QString(node.attribute("ID"));
    }

    const auto node_type = getNodeTypeFromString(node_name);

    ParameterWidgetCreators parameters;

    QDomNamedNodeMap attributes = node.attributes ();
    for (int i=0; i< attributes.size(); i++ )
    {
        QDomAttr attr = attributes.item(i).toAttr();

        QString attr_name( attr.name() );
        if(attr_name != "ID" && attr_name != "name")
        {
            TreeNodeModel::Param model_param;
            model_param.label = attr_name;
            model_param.value = attr.value();

            auto widget_creator = buildWidgetCreator( model_param );
            parameters.push_back(widget_creator);
            model_params.push_back( std::move(model_param) );
        }
    }

    return { ID, node_type, model_params };
}

//------------------------------------------------------------------

TreeNodeModels ReadTreeNodesModel(const QDomElement &root)
{
    TreeNodeModels models;
    using QtNodes::DataModelRegistry;

    auto model_root = root.firstChildElement("TreeNodesModel");

    if( !model_root.isNull() )
    {
        for( QDomElement node = model_root.firstChildElement();
             !node.isNull();
             node = node.nextSiblingElement() )
        {
            auto model = buildTreeNodeModel(node);
            models.insert( {model.registration_ID, model} );
        }
    }

    std::function<void(QDomElement)> recursiveStep;
    recursiveStep = [&](QDomElement node)
    {
        auto model = buildTreeNodeModel(node);
        models.insert( {model.registration_ID, model} );

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
        element = doc.createElement( toStr(bt_node->nodeType()) );
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

    auto parameters = bt_node->getCurrentParameters();
    for(const auto& param: parameters)
    {
        element.setAttribute( param.label.toStdString().c_str(),
                               param.value.toStdString().c_str() );
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

    auto ChildrenCount = [](const QDomElement& parent_node) {
        int count = 0;
        for (auto node = parent_node.firstChildElement();
             !node.isNull();
             node = node.nextSiblingElement())
        {
            count++;
        }
        return count;
    };

    //-----------------------------

    QDomElement xml_root = doc.documentElement();

    if ( xml_root.isNull() || xml_root.nodeName() != "root")
    {
        error_messages.emplace_back("The XML must have a root node called <root>");
        return false;
    }
    //-------------------------------------------------
    auto meta_root = xml_root.firstChildElement("TreeNodesModel");
    bool has_sibling = !meta_root.isNull() && !meta_root.nextSiblingElement("TreeNodesModel").isNull();

    if (has_sibling)
    {
        AppendError(meta_root.nextSiblingElement("TreeNodesModel").lineNumber(),
                    " Only a single node <TreeNodesModel> is supported");
    }
    if ( !meta_root.isNull() )
    {
        // not having a MetaModel is not an error. But consider that the
        // Graphical editor needs it.
        for (auto node = xml_root.firstChildElement();
             !node.isNull();
             node = node.nextSiblingElement())
        {
            QString name = node.nodeName();
            if ( name == "Action" || name == "Decorator" ||
                 name == "SubTree" || name == "Condition")
            {
                if ( !node.hasAttribute("ID") )
                {
                    AppendError(node.lineNumber(), "Error at line %d: -> The attribute [ID] is "
                                                    "mandatory");
                }
                QString ID = node.attribute("ID");

                for (auto param_node = xml_root.firstChildElement("Parameter");
                     !param_node.isNull();
                     param_node = param_node.nextSiblingElement("Parameter"))
                {
                    if (! node.hasAttribute("label") || !node.hasAttribute("type") )
                    {
                        AppendError(node.lineNumber(),
                                    "The node <Parameter> requires the attributes [type] and "
                                    "[label]");
                    }
                    QString label = node.attribute("label");
                    QString type  = node.attribute("type");
                }
            }
        }
    }

    //-------------------------------------------------

    // function to be called recursively
    std::function<void(const QDomElement&)> recursiveStep;

    recursiveStep = [&](const QDomElement& node) {
        const int children_count = ChildrenCount(node);
        QString name = node.nodeName();
        if (name == "Decorator")
        {
            if (children_count != 1)
            {
                AppendError(node.lineNumber(), "The node <Decorator> must have exactly 1 child");
            }
            if (!node.hasAttribute("ID"))
            {
                AppendError(node.lineNumber(), "The node <Decorator> must have the attribute "
                                                "[ID]");
            }
        }
        else if (name == "Action")
        {
            if (children_count != 0)
            {
                AppendError(node.lineNumber(), "The node <Action> must not have any child");
            }
            if (!node.hasAttribute("ID"))
            {
                AppendError(node.lineNumber(), "The node <Action> must have the attribute [ID]");
            }
        }
        else if (name == "Condition")
        {
            if (children_count != 0)
            {
                AppendError(node.lineNumber(), "The node <Condition> must not have any child");
            }
            if (!node.hasAttribute("ID"))
            {
                AppendError(node.lineNumber(), "The node <Condition> must have the attribute "
                                                "[ID]");
            }
        }
        else if (name == "Sequence" || name == "SequenceStar" ||
                 name == "Fallback" || name == "FallbackStar")
        {
            if (children_count == 0)
            {
                AppendError(node.lineNumber(), "A Control node must have at least 1 child");
            }
        }
        else if (name == "SubTree")
        {
            if (children_count > 0)
            {
                AppendError(node.lineNumber(), "The <SubTree> node must have no children");
            }
            if (!node.hasAttribute("ID"))
            {
                AppendError(node.lineNumber(), "The node <SubTree> must have the attribute [ID]");
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
                AppendError(node.lineNumber(), (std::string("Node not recognized: ") + name.toStdString()).c_str() );
            }
        }
        //recursion
        for (auto child = node.firstChildElement(); !child.isNull();
             child = child.nextSiblingElement())
        {
            recursiveStep(child);
        }
    };

    std::vector<std::string> tree_names;
    int tree_count = 0;

    for (auto bt_root = xml_root.firstChildElement("BehaviorTree"); !bt_root.isNull();
         bt_root = bt_root.nextSiblingElement("BehaviorTree"))
    {
        tree_count++;
        if (bt_root.hasAttribute("ID"))
        {
            tree_names.push_back( bt_root.attribute("ID").toStdString() );
        }
        if (ChildrenCount(bt_root) != 1)
        {
            AppendError(bt_root.lineNumber(), "The node <BehaviorTree> must have exactly 1 child");
        }
        else
        {
            recursiveStep( bt_root.firstChildElement() );
        }
    }

    if (xml_root.hasAttribute("main_tree_to_execute"))
    {
        std::string main_tree = xml_root.attribute("main_tree_to_execute").toStdString();
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


