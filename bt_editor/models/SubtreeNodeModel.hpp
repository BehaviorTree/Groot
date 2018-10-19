#ifndef SUBTREE_NODEMODEL_HPP
#define SUBTREE_NODEMODEL_HPP

#include "BehaviorTreeNodeModel.hpp"
#include <QPushButton>

class SubtreeNodeModel : public BehaviorTreeDataModel
{
    Q_OBJECT
public:

    SubtreeNodeModel(const QString& subtree_ID,
                     const ParameterWidgetCreators &creators);

    ~SubtreeNodeModel() override = default;

    unsigned int  nPorts(PortType portType) const override {
        return portType == PortType::In ? 1:0;
    }

    virtual const char* className() const final { return Name(); }

    static const char* Name() { return "SubTree";  }

    std::pair<QString, QColor> caption() const override
    { return { name(), QtNodes::NodeStyle().FontColor}; }

    NodeType nodeType() const final { return NodeType::SUBTREE; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/subtree.svg");
    }

signals:
    void expandButtonPushed();

private:
    QPushButton* _expand_button;

};

static const QString EXPANDED_SUFFIX("[expanded]");

class SubtreeExpandedNodeModel : public BehaviorTreeDataModel
{
    Q_OBJECT
public:

    SubtreeExpandedNodeModel(const QString& subtree_ID,
                             const ParameterWidgetCreators &creators);

    ~SubtreeExpandedNodeModel() override = default;

    unsigned int  nPorts(PortType) const override { return 1; }

    virtual const char* className() const final { return Name(); }

    static const char* Name() { return "SubTreeExpanded";  }

    std::pair<QString, QColor> caption() const override
    { return { "SubTreeExpanded", QtNodes::NodeStyle().FontColor}; }

    ConnectionPolicy portOutConnectionPolicy(PortIndex) const final;

    NodeType nodeType() const final { return NodeType::SUBTREE; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/subtree.svg");
    }

signals:
    void collapseButtonPushed();

private:

    QPushButton* _collapse_button;

};



#endif // SUBTREE_NODEMODEL_HPP
