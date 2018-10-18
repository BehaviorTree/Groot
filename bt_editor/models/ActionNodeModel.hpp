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

    std::pair<QString,QColor> caption() const override { return {instanceName(),"#ddff55"}; }

    virtual NodeType nodeType() const final { return NodeType::ACTION; }

    virtual QSvgRenderer* icon() const override {
        static QSvgRenderer* renderer = createSvgRenderer(":/icons/svg/letter_A.svg");
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

    std::pair<QString,QColor> caption() const override { return {instanceName(),"#44bb44"}; }

    virtual NodeType nodeType() const final { return NodeType::CONDITION; }

    virtual QSvgRenderer* icon() const override {
        static QSvgRenderer* renderer = createSvgRenderer(":/icons/svg/letter_C.svg");
        return renderer;
    }

private:
};


class ActionSuccess : public ActionNodeModel
{
public:
    ActionSuccess(): ActionNodeModel( "AlwaysSuccess",ParameterWidgetCreators()){}

    static const char* Name() {  return "AlwaysSuccess"; }

    std::pair<QString, QColor> caption() const override  { return { "SUCCESS", Qt::green}; }

    virtual QSvgRenderer* icon() const override { return nullptr; }
};

class ActionFailure : public ActionNodeModel
{
public:
    ActionFailure(): ActionNodeModel( "AlwaysFailure",ParameterWidgetCreators()){}

    static const char* Name() {  return "AlwaysFailure"; }

    std::pair<QString, QColor> caption() const override { return { "FAILURE", Qt::red}; }

    virtual QSvgRenderer* icon() const override { return nullptr; }
};


