#pragma once

#include "BehaviorTreeNodeModel.hpp"


class ActionNodeModel : public BehaviorTreeDataModel
{
public:
    ActionNodeModel(const QString& action_ID, const TreeNodeModel &model);

    virtual ~ActionNodeModel()  = default;

    virtual unsigned int  nPorts(PortType portType) const override;

    const char* className() const final { return ActionNodeModel::Name();  }

    static const char* Name() { return "Action"; }

    std::pair<QString,QColor> caption() const override { return {instanceName(),"#ddff55"}; }

    virtual NodeType nodeType() const final { return NodeType::ACTION; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/letter_A.svg");
    }

private:

};


class ConditionNodeModel : public BehaviorTreeDataModel
{
public:
    ConditionNodeModel(const QString& action_ID, const TreeNodeModel &model);

    virtual ~ConditionNodeModel()  = default;

    unsigned int  nPorts(PortType portType) const override;

    const char* className() const final { return ConditionNodeModel::Name();  }

    static const char* Name() { return "Condition"; }

    std::pair<QString,QColor> caption() const override { return {instanceName(),"#44bb44"}; }

    virtual NodeType nodeType() const final { return NodeType::CONDITION; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/letter_C.svg");
    }

private:
};


class ActionSuccess : public ActionNodeModel
{
public:
    ActionSuccess(): ActionNodeModel( Name(), TreeNodeModel()){}

    static const char* Name() {  return "AlwaysSuccess"; }

    std::pair<QString, QColor> caption() const override  { return { "SUCCESS", Qt::green}; }

};

class ActionFailure : public ActionNodeModel
{
public:
    ActionFailure(): ActionNodeModel( Name(), TreeNodeModel()){}

    static const char* Name() {  return "AlwaysFailure"; }

    std::pair<QString, QColor> caption() const override { return { "FAILURE", "#ff2222"}; }

};

class ActionSetBlackboard : public ActionNodeModel
{
public:
    ActionSetBlackboard(): ActionNodeModel( Name(), NodeModel()){}

    static const TreeNodeModel& NodeModel()
    {
        static TreeNodeModel model = { NodeType::ACTION, { {"key", "key" }, {"value", "value" } } };
        return model;
    }

    static const char* Name() {  return "SetBlackboard"; }

    std::pair<QString, QColor> caption() const override { return { Name(), QtNodes::NodeStyle().FontColor}; }
};


