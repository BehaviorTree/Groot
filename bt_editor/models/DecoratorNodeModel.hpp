#ifndef DECORATORNODEMODEL_HPP
#define DECORATORNODEMODEL_HPP

#include <QLineEdit>
#include "BehaviorTreeNodeModel.hpp"


class DecoratorNodeModel : public BehaviorTreeDataModel
{
public:
    DecoratorNodeModel(QString decorator_ID, const TreeNodeModel &model);

    virtual ~DecoratorNodeModel() = default;

    unsigned int  nPorts(PortType) const final { return 1; }

    ConnectionPolicy portOutConnectionPolicy(PortIndex) const final { return ConnectionPolicy::One; }

    virtual const char* className() const final {  return "Decorator"; }

    std::pair<QString, QColor> caption() const override
    { return { "Decorator", QtNodes::NodeStyle().FontColor}; }

    virtual NodeType nodeType() const final { return NodeType::DECORATOR; }
};

class RetryNodeModel : public DecoratorNodeModel
{
public:
    RetryNodeModel();

    static const TreeNodeModel& NodeModel()
    {
        static TreeNodeModel model = { NodeType::DECORATOR, { {tr("num_attempts"), "1" } } };
        return model;
    }

    static const char* Name() {  return "RetryUntilSuccesful"; }

    std::pair<QString, QColor> caption() const override
    { return { Name(), QtNodes::NodeStyle().FontColor}; }

    virtual QString captionIicon() const override {
        return ":/icons/svg/retry.svg";
    }
};


class InverterNodeModel : public DecoratorNodeModel
{
public:
    InverterNodeModel();

    static const char* Name() {  return "Inverter"; }

    std::pair<QString, QColor> caption() const override
    { return { Name(), QtNodes::NodeStyle().FontColor}; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/not_equal.svg");
    }

};

class RepeatNodeModel : public DecoratorNodeModel
{
public:
    RepeatNodeModel();

    static const TreeNodeModel& NodeModel()
    {
        static TreeNodeModel model = { NodeType::DECORATOR, { {tr("num_cycles"), "1"} } };
        return model;
    }

    static const char* Name() {  return "Repeat"; }

    std::pair<QString, QColor> caption() const override
    { return { Name(), QtNodes::NodeStyle().FontColor}; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/repeat.svg");
    }
};


class BlackboardConditionModel : public DecoratorNodeModel
{
public:
    BlackboardConditionModel();

    static const TreeNodeModel& NodeModel()
    {
        static TreeNodeModel model = { NodeType::DECORATOR, { {"key", "key"}, {"expected", "value"} } };
        return model;
    }
    static const char* Name() {  return "BB_Precondition"; }

    std::pair<QString, QColor> caption() const override
    { return { Name(), QtNodes::NodeStyle().FontColor}; }
};

class TimeoutModel : public DecoratorNodeModel
{
public:
    TimeoutModel();

    static const TreeNodeModel& NodeModel()
    {
        static TreeNodeModel model = { NodeType::DECORATOR, { {"msec", "0"} } };
        return model;
    }
    static const char* Name() {  return "Timeout"; }

    std::pair<QString, QColor> caption() const override
    { return { Name(), QtNodes::NodeStyle().FontColor}; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/timeout.svg");
    }
};

class ForceSuccess : public DecoratorNodeModel
{
public:
    ForceSuccess();

    static const char* Name() {  return "ForceSuccess"; }

    std::pair<QString, QColor> caption() const override  { return { Name(), Qt::green}; }
};

class ForceFailure : public DecoratorNodeModel
{
public:
    ForceFailure();

    static const char* Name() {  return "ForceFailure"; }

    std::pair<QString, QColor> caption() const override  { return { Name(), "#ff2222"}; }
};



#endif // DECORATORNODEMODEL_HPP
