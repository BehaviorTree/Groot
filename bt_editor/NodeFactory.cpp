#include "NodeFactory.hpp"
#include <QFile>
#include <QDomElement>
#include <QLineEdit>
#include <QComboBox>
#include <QDebug>
#include "ActionNodeModel.hpp"
#include "DecoratorNodeModel.hpp"
#include "SubtreeNodeModel.hpp"

bool NodeFactory::loadMetaModelFromFile(QString filename, QtNodes::DataModelRegistry &registry)
{
    QFile file(filename);
    if (!file.open(QFile::ReadOnly | QFile::Text)) {
        throw std::runtime_error(std::string("can't load the file ") + filename.toStdString());
    }
    QString errorStr;
    int errorLine, errorColumn;

    QDomDocument domDocument;

    if (!domDocument.setContent(&file, true, &errorStr, &errorLine, &errorColumn))
    {
        QString error_msg = QString("XML Parse error of file %1 at line %2: %3")
                                 .arg( filename).arg(errorLine).arg(errorStr);
        throw std::runtime_error(error_msg.toStdString());
    }
    QDomElement root = domDocument.documentElement();
    loadMetaModelFromXML(root, registry);
    return true;
}

bool NodeFactory::loadModelFromString(QString text, QtNodes::DataModelRegistry &registry)
{
    QString errorStr;
    int errorLine, errorColumn;

    QDomDocument domDocument;

    if (!domDocument.setContent(text, true, &errorStr, &errorLine, &errorColumn))
    {
        QString error_msg = QString("XML Parse error at line %1: %2").arg(errorLine).arg(errorStr);
        throw std::runtime_error(error_msg.toStdString());
    }

//    QDomNodeList root_children = domDocument.childNodes();

//    loadMetaModelFromXML(root_children);
    return true;
}

bool NodeFactory::loadMetaModelFromXML(const QDomElement& root,
                                       QtNodes::DataModelRegistry& registry)

{
    std::map<QString, QSet<QString>> registered_models;

    auto isModelRegistered = [&registered_models](const QString& category, const QString& ID)
    {
      auto it = registered_models.find( category );
      if( it == registered_models.end()) return false;
      const auto& models = it->second;
      return (models.find( ID ) != models.end());
    };

    auto registerAction = [&isModelRegistered, &registered_models, &registry](QString ID)
    {
      if( !isModelRegistered( "Action", ID) )
      {
        registry.registerModel<ActionNodeModel>( "Action", [ID]() {
          return std::make_unique<ActionNodeModel>( ID );
        } );
        registered_models["Action"].insert( std::move(ID) );
      }
    };

    auto registerDecorator = [&isModelRegistered, &registered_models, &registry](const QString& ID)
    {
      if( !isModelRegistered( "Decorator", ID) )
      {
        registry.registerModel<DecoratorNodeModel>(  "Decorator", [ID]() {
          return std::make_unique<DecoratorNodeModel>( ID );
        } );
        registered_models["Decorator"].insert(ID);
      }
    };

    auto registerSubtree = [&isModelRegistered, &registered_models, &registry](const QString& ID)
    {
      if( !isModelRegistered( "SubTree", ID) )
      {
        registry.registerModel<SubtreeNodeModel>( "SubTree", [ID]() {
          return std::make_unique<SubtreeNodeModel>( ID );
        } );
        registered_models["SubTree"].insert(ID);
      }
    };

    //--------------------------------------
    QDomElement metamodel_root = root.firstChildElement("BehaviorTreeMetaModel");

    // Recursively add any action or decorator that is missing. Note that no ParamWidget can be added
    std::function<void(const QDomElement&)> recursiveStep;

    recursiveStep = [&](const QDomElement& element)
    {
        const QString element_type = element.tagName();
        const QString id = element.attribute("ID");

        if( element_type == "Action" )
        {
          if( element.hasAttribute("ID") == false)
          {
            throw std::runtime_error("The Node Action must have the attribute [ID]");
          }
          registerAction(id);
        }
        else if( element_type == "Decorator")
        {
          if( element.hasAttribute("ID") == false)
          {
            throw std::runtime_error("The Node Decorator must have the attribute [ID]");
          }
          registerDecorator(id);
        }
        else if( element_type == "SubTree")
        {
          if( element.hasAttribute("ID") == false)
          {
            throw std::runtime_error("The Node SubTree must have the attribute [ID]");
          }
          registerSubtree(id);
        }

        for(QDomElement child = element.firstChildElement();
            child.isNull() == false;
            child = child.nextSiblingElement() )
        {
            recursiveStep(child);
        }
    };
    //-------------------------------------------------
    // READ the metamodel section if present
    if( !metamodel_root.isNull() )
    {
        NodeFactory::get().loadMetaModelParameters(metamodel_root, registry);

        for(QDomElement child = metamodel_root.firstChildElement();
            child.isNull() == false;
            child = child.nextSiblingElement() )
        {
            recursiveStep(child);
        }
    }
    //-------------------------------------------------
    // add the SubTreeDefinitions
    for(QDomElement element = root.firstChildElement("BehaviorTree");
        element.isNull() == false;
        element = element.nextSiblingElement("BehaviorTree") )
    {
      if( element.hasAttribute("ID") == false)
      {
        throw std::runtime_error("The Node SubTreeDefinition must have the attribute [ID]");
      }
      const QString ID = element.attribute("ID");
      registerSubtree( ID );
    }
    //-------------------------------------------------

    // Do the same recursion inside the SubTreeDefinitions
    for(QDomElement subtree = root.firstChildElement("BehaviorTree");
        subtree.isNull() == false;
        subtree = subtree.nextSiblingElement("BehaviorTree") )
    {
      for(QDomElement child = subtree.firstChildElement();
          child.isNull() == false;
          child = child.nextSiblingElement() )
      {
        recursiveStep(child);
      }
    }

    return true;
}

void NodeFactory::loadMetaModelParameters(const QDomElement& metamodel_root, QtNodes::DataModelRegistry& registry)
{
    const QString type_name[3]  = { "Action", "Decorator", "SubTree" };
    ParametersModel* parameter_model[3] = { &NodeFactory::get()._action_parameter_model,
                                            &NodeFactory::get()._decorator_parameter_model,
                                            &NodeFactory::get()._subtree_parameter_model};
    for(int i=0; i<3; i++)
    {
        for (QDomElement  elem_action = metamodel_root.firstChildElement( type_name[i] )  ;
             elem_action.isNull() == false;
             elem_action = elem_action.nextSiblingElement( type_name[i] ) )
        {
            if( !elem_action.hasAttribute("ID") )
            {
                throw  std::runtime_error("element <Action>must have the attribute 'ID'");
            }

            QString ID = elem_action.attribute("ID");

            (*parameter_model[i])[ID] = {};

            for (QDomElement  elem_parameter = elem_action.firstChildElement(  "Parameter" )  ;
                 elem_parameter.isNull() == false;
                 elem_parameter = elem_parameter.nextSiblingElement( "Parameter" ) )
            {
                if( !elem_parameter.hasAttribute("label") || !elem_parameter.hasAttribute("type") )
                {
                    throw  std::runtime_error("element <Parameter> needs an attribute 'type' and 'label'");
                }
                ParamWidget pw;
                pw.label = elem_parameter.attribute("label");
                QString widget_type = elem_parameter.attribute("type");

                if( widget_type == "Text"){
                    pw.instance_factory = instanceFactoryText();
                }
                else if( widget_type == "Int"){
                    pw.instance_factory = instanceFactoryInt();
                }
                else if( widget_type == "Double"){
                    pw.instance_factory = instanceFactoryDouble();
                }
                else if( widget_type == "Combo"){

                    if( !elem_parameter.hasAttribute("options"))
                    {
                        throw  std::runtime_error("A parameter marked ad Combo must have the field [options]");
                    }

                    QString options = elem_parameter.attribute("options");
                    QStringList option_list = options.split(";", QString::SkipEmptyParts);
                    pw.instance_factory = instanceFactoryCombo(option_list);
                }
                else{
                    throw  std::runtime_error("Attribute 'type' of element <Parameter>"
                                              " must be either: Text, Int, Double or Combo");
                }
                (*parameter_model[i])[ID].push_back( pw );
            }
        }
    }
}



const NodeFactory::ParametersModel &NodeFactory::getActionParameterModel()
{
    return NodeFactory::get()._action_parameter_model;
}

const NodeFactory::ParametersModel &NodeFactory::getDecoratorParameterModel()
{
  return NodeFactory::get()._decorator_parameter_model;
}

const NodeFactory::ParametersModel &NodeFactory::getSubtreeParameterModel()
{
  return NodeFactory::get()._subtree_parameter_model;
}

std::function<QWidget *()> NodeFactory::instanceFactoryText()
{
  return [](){
    QLineEdit* line = new QLineEdit();
    line->setAlignment( Qt::AlignHCenter);
    return line;
  };
}

std::function<QWidget *()> NodeFactory::instanceFactoryInt()
{
  return [](){
    QLineEdit* line = new QLineEdit();
    line->setValidator( new QIntValidator( line ));
    line->setAlignment( Qt::AlignHCenter);
    return line;
  };
}

std::function<QWidget *()> NodeFactory::instanceFactoryDouble()
{
  return [](){
    QLineEdit* line = new QLineEdit();
    line->setValidator( new QDoubleValidator( line ));
    line->setAlignment( Qt::AlignHCenter);
    return line;
  };
}

std::function<QWidget *()> NodeFactory::instanceFactoryCombo(QStringList options)
{
  return [options](){
    QComboBox* combo = new QComboBox();
    combo->addItems(options);
    return combo;
  };
}

void NodeFactory::clear()
{
    NodeFactory::get()._action_parameter_model.clear();
    NodeFactory::get()._decorator_parameter_model.clear();
}

NodeFactory &NodeFactory::get(){
    static NodeFactory instance;
    return instance;
}
