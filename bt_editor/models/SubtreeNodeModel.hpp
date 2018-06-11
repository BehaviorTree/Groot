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

    unsigned int  nPorts(PortType portType) const override
    {
        return portType == PortType::In ? 1:0;
    }

    virtual const char* className() const final
    {
        return Name();
    }

    static const char* Name()
    {
        return "SubTree";
    }

    QString caption() const override {
        return "SubTree";
    }

    NodeType nodeType() const final { return NodeType::SUBTREE; }

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

    unsigned int  nPorts(PortType) const override {
        return 1;
    }

    virtual const char* className() const final
    {
        return Name();
    }

    static const char* Name()
    {
        return "SubTreeExpanded";
    }

    QString caption() const override {
        return "SubTreeExpanded";
    }

    ConnectionPolicy portOutConnectionPolicy(PortIndex) const final
    {
        return ConnectionPolicy::One;
    }

    NodeType nodeType() const final { return NodeType::SUBTREE; }

signals:
    void collapseButtonPushed();

private:

    QPushButton* _collapse_button;

};



#endif // SUBTREE_NODEMODEL_HPP
