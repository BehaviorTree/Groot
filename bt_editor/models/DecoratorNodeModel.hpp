#ifndef DECORATORNODEMODEL_HPP
#define DECORATORNODEMODEL_HPP

#include <QLineEdit>
#include "BehaviorTreeNodeModel.hpp"


class DecoratorNodeModel : public BehaviorTreeDataModel
{
public:
    DecoratorNodeModel(QString decorator_ID, const ParameterWidgetCreators &parameters);

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
    RetryNodeModel(const ParameterWidgetCreators &parameters =
    { buildWidgetCreator( NodeModel().params.front() ) } );

    static const TreeNodeModel& NodeModel()
    {
        static TreeNodeModel model = { NodeType::DECORATOR, { {tr("num_attempts"), "1" } } };
        return model;
    }

    static const char* Name() {  return "RetryUntilSuccesful"; }

    std::pair<QString, QColor> caption() const override
    { return { Name(), QtNodes::NodeStyle().FontColor}; }

    virtual QSvgRenderer* icon() const override {
        static QSvgRenderer* renderer = createSvgRenderer(":/icons/svg/retry.svg");
        return renderer;
    }
};


class NegationNodeModel : public DecoratorNodeModel
{
public:
    NegationNodeModel(const ParameterWidgetCreators &parameters = ParameterWidgetCreators());

    static const char* Name() {  return "Negation"; }

    std::pair<QString, QColor> caption() const override
    { return { Name(), QtNodes::NodeStyle().FontColor}; }

    virtual QSvgRenderer* icon() const override {
        static QSvgRenderer* renderer = createSvgRenderer(":/icons/svg/not_equal.svg");
        return renderer;
    }
};

class RepeatNodeModel : public DecoratorNodeModel
{
public:
    RepeatNodeModel(const ParameterWidgetCreators &parameters =
    { buildWidgetCreator( NodeModel().params.front() ) });

    static const TreeNodeModel& NodeModel()
    {
        static TreeNodeModel model = { NodeType::DECORATOR, { {tr("num_cycles"), "1"} } };
        return model;
    }

    static const char* Name() {  return "Repeat"; }

    std::pair<QString, QColor> caption() const override
    { return { Name(), QtNodes::NodeStyle().FontColor}; }

    virtual QSvgRenderer* icon() const override {
        static QSvgRenderer* renderer = createSvgRenderer(":/icons/svg/repeat.svg");
        return renderer;
    }
};


#endif // DECORATORNODEMODEL_HPP
