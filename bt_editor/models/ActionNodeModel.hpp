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

    std::pair<QString,QColor> caption() const override { return {"Action","#ffff66"}; }

    virtual NodeType nodeType() const final { return NodeType::ACTION; }

    virtual QSvgRenderer* icon() const override {
        static QSvgRenderer* renderer = new QSvgRenderer(tr(":/icons/svg/exclaimation_mark.svg"));
        return renderer;
    }

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

    std::pair<QString,QColor> caption() const override { return {"Condition","#00cc99"}; }

    virtual NodeType nodeType() const final { return NodeType::CONDITION; }

private:

};
