#ifndef NODE_FACTORY_HPP
#define NODE_FACTORY_HPP

#include <QString>
#include <QWidget>
#include <functional>
#include <nodes/DataModelRegistry>

//class NodeFactory
//{
//    NodeFactory() {}
//public:

//    struct ParamWidget{
//        QString label;
//        std::function<QWidget*()> instance_factory;
//    };

//    typedef std::map<QString, std::vector<ParamWidget>> ParametersModel;

//    static void clear();

//    static NodeFactory& get();

//    static bool loadMetaModelFromFile(QString filename, QtNodes::DataModelRegistry &registry);

//    static bool loadModelFromString(QString text, QtNodes::DataModelRegistry &registry);

//    static bool loadMetaModelFromXML(const QDomElement &root, QtNodes::DataModelRegistry &registry);

//    static const ParametersModel& getActionParameterModel();

//    static const ParametersModel& getDecoratorParameterModel();

//    static const ParametersModel& getSubtreeParameterModel();

//    static void addActionToModel(QString name, std::vector<ParamWidget> parameters_widgets);

//    static void addDecoratorToModel(QString name, std::vector<ParamWidget> parameters_widgets);

//    static std::function<QWidget*()> instanceFactoryText();
//    static std::function<QWidget*()> instanceFactoryInt();
//    static std::function<QWidget*()> instanceFactoryDouble();
//    static std::function<QWidget*()> instanceFactoryCombo(QStringList options);


//private:
//    ParametersModel _action_parameter_model;
//    ParametersModel _decorator_parameter_model;
//    ParametersModel _subtree_parameter_model;

//    void loadMetaModelParameters(const QDomElement& metamodel_root, QtNodes::DataModelRegistry &registry);
//};

#endif // NODE_FACTORY_HPP
