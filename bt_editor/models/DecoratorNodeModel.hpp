#ifndef DECORATORNODEMODEL_HPP
#define DECORATORNODEMODEL_HPP

#include "BehaviorTreeNodeModel.hpp"

class DecoratorNodeModel : public BehaviorTreeDataModel
{
public:
    DecoratorNodeModel(QString decorator_ID, const ParameterWidgetCreators &parameters);

    virtual ~DecoratorNodeModel() = default;

    unsigned int  nPorts(PortType) const override final
    { return 1; }

    ConnectionPolicy portOutConnectionPolicy(PortIndex) const override final
    {
        return ConnectionPolicy::One;
    }

    virtual const char* className() const override final
    {
      return "Decorator";
    }

    QString caption() const override {
      return "Decorator";
    }

     virtual NodeType nodeType() const override final { return NodeType::DECORATOR; }

private:

};


#endif // DECORATORNODEMODEL_HPP
