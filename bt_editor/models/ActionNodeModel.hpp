#pragma once

#include "BehaviorTreeNodeModel.hpp"


class ActionNodeModel : public BehaviorTreeDataModel
{
public:
    ActionNodeModel(const QString& action_ID, const ParameterWidgetCreators &creators);

    virtual ~ActionNodeModel()  = default;

    virtual unsigned int  nPorts(PortType portType) const override;

    const char* className() const final { return ActionNodeModel::Name();  }

    static const char* Name() { return "Action"; }

    QString caption() const override { return "Action"; }

    virtual NodeType nodeType() const final { return NodeType::ACTION; }

private:

};


class ConditionNodeModel : public BehaviorTreeDataModel
{
public:
    ConditionNodeModel(const QString& action_ID, const ParameterWidgetCreators &creators);

    virtual ~ConditionNodeModel()  = default;

    unsigned int  nPorts(PortType portType) const override;

    const char* className() const final { return ConditionNodeModel::Name();  }

    static const char* Name() { return "Condition"; }

    QString caption() const override { return "Condition"; }

    virtual NodeType nodeType() const final { return NodeType::CONDITION; }

private:

};
