#ifndef DECORATORNODEMODEL_HPP
#define DECORATORNODEMODEL_HPP

#include "BehaviorTreeNodeModel.hpp"

class DecoratorNodeModel : public BehaviorTreeNodeModel
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

private:

};


#endif // DECORATORNODEMODEL_HPP
