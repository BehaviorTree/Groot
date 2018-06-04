#pragma once

#include "BehaviorTreeNodeModel.hpp"


class ActionNodeModel : public BehaviorTreeDataModel
{
public:
    ActionNodeModel(const QString& action_ID, const ParameterWidgetCreators &creators);

    virtual ~ActionNodeModel()  = default;

    virtual unsigned int  nPorts(PortType portType) const override;

    virtual const char* className() const override final
    {
        return ActionNodeModel::Name();
    }
    static const char* Name() { return "Action"; }

    QString caption() const override {
        return "Action";
    }

    virtual NodeType nodeType() const override final { return NodeType::ACTION; }

private:

};

