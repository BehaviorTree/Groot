#ifndef SUBTREE_NODEMODEL_HPP
#define SUBTREE_NODEMODEL_HPP

#include "BehaviorTreeNodeModel.hpp"
#include <QPushButton>
#include <memory.hpp>

class SubtreeNodeModel : public BehaviorTreeDataModel
{
    Q_OBJECT
public:

    SubtreeNodeModel(const QString& subtree_ID,
                     const ParameterWidgetCreators &creators);

    unsigned int  nPorts(PortType portType) const override {
        return portType == PortType::In ? 1:0;
    }

    const char* className() const final { return Name(); }

    static const char* Name() { return "SubTree";  }

    std::pair<QString, QColor> caption() const override
    { return { "SubTree", QtNodes::NodeStyle().FontColor}; }

    NodeType nodeType() const final { return NodeType::SUBTREE; }

    QSvgRenderer* icon() const override { return _renderer.get(); }

signals:
    void expandButtonPushed();

private:
    detail::unique_qptr<QSvgRenderer> _renderer;
};

static const QString EXPANDED_SUFFIX("[expanded]");

class SubtreeExpandedNodeModel : public BehaviorTreeDataModel
{
    Q_OBJECT
public:

    SubtreeExpandedNodeModel(const QString& subtree_ID,
                             const ParameterWidgetCreators &creators);

    unsigned int  nPorts(PortType) const override { return 1; }

    virtual const char* className() const final { return Name(); }

    static const char* Name() { return "SubTreeExpanded";  }

    std::pair<QString, QColor> caption() const override
    { return { "SubTreeExpanded", QtNodes::NodeStyle().FontColor}; }

    ConnectionPolicy portOutConnectionPolicy(PortIndex) const final;

    NodeType nodeType() const final { return NodeType::SUBTREE; }

    QSvgRenderer* icon() const override { return _renderer.get(); }

signals:
    void collapseButtonPushed();

private:
    detail::unique_qptr<QSvgRenderer> _renderer;
};

#endif // SUBTREE_NODEMODEL_HPP
