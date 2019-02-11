#pragma once

#include "BehaviorTreeNodeModel.hpp"

/*
class ActionNodeModel : public BehaviorTreeDataModel
{
public:
    ActionNodeModel(const BT_NodeModel &model);

    virtual ~ActionNodeModel()  = default;

    virtual unsigned int  nPorts(PortType portType) const override;

    static const char* Name() { return "Action"; }

    std::pair<QString,QColor> caption() const override;

    virtual NodeType nodeType() const final { return NodeType::ACTION; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/letter_A.svg");
    }

    virtual void setInstanceName(const QString& name) override;

private:

};


class ConditionNodeModel : public BehaviorTreeDataModel
{
public:
    ConditionNodeModel(const BT_NodeModel &model);

    virtual ~ConditionNodeModel()  = default;

    unsigned int  nPorts(PortType portType) const override;

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
    ActionSuccess();

    static const char* Name() {  return "AlwaysSuccess"; }

    static TreeNodeModel NodeModel(){
        return { Name(), NodeType::ACTION, {} };
    }

    std::pair<QString, QColor> caption() const override  { return { "Success", Qt::green}; }

};

class ActionFailure : public ActionNodeModel
{
public:
    ActionFailure();

    static const char* Name() {  return "AlwaysFailure"; }

    static TreeNodeModel NodeModel(){
        return { Name(), NodeType::ACTION, {} };
    }

    std::pair<QString, QColor> caption() const override { return { "Failure", "#ff2222"}; }

};

class ActionSetBlackboard : public ActionNodeModel
{
public:
    ActionSetBlackboard(): ActionNodeModel(NodeModel()){}

    static TreeNodeModel NodeModel()
    {
        return { Name(), NodeType::ACTION, { {"key", "key" }, {"value", "value" } } };
    }

    static const char* Name() {  return "SetBlackboard"; }

    std::pair<QString, QColor> caption() const override { return { Name(), QtNodes::NodeStyle().FontColor}; }


    virtual QString captionIicon() const override {
        return(":/icons/svg/edit_list.svg");
    }
};
*/

