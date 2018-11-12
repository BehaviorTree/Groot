#ifndef SUBTREE_NODEMODEL_HPP
#define SUBTREE_NODEMODEL_HPP

#include "BehaviorTreeNodeModel.hpp"
#include <QPushButton>

class SubtreeNodeModel : public BehaviorTreeDataModel
{
    Q_OBJECT
public:

    SubtreeNodeModel(const QString& subtree_ID,
                     const TreeNodeModel& model);

    ~SubtreeNodeModel() override = default;

    void setExpanded(bool expand);

    bool expanded() const { return _expanded; }

    unsigned int  nPorts(PortType portType) const override {
        int out_port = _expanded ? 1 : 0;
        return portType == PortType::In ? 1:out_port;
    }

    virtual const char* className() const final { return Name(); }

    static const char* Name() { return "SubTree";  }

    std::pair<QString, QColor> caption() const override
    { return { registrationName(), QtNodes::NodeStyle().FontColor}; }

    NodeType nodeType() const final { return NodeType::SUBTREE; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/subtree.svg");
    }

    QPushButton* expandButton() { return _expand_button; }

signals:
    void expandButtonPushed();

private:
    QPushButton* _expand_button;
    bool _expanded;

};
/*
static const QString SUBTREE_EXPANDED_SUFFIX("[expanded]");

class SubtreeExpandedNodeModel : public BehaviorTreeDataModel
{
    Q_OBJECT
public:

    SubtreeExpandedNodeModel(const QString& base_subtree_ID,
                             const TreeNodeModel& model);

    ~SubtreeExpandedNodeModel() override = default;

    unsigned int nPorts(PortType) const override { return 1; }

    virtual const char* className() const final { return Name(); }

    static const char* Name() { return "SubTreeExpanded";  }

    std::pair<QString, QColor> caption() const override
    { return { _base_ID, QtNodes::NodeStyle().FontColor}; }

    ConnectionPolicy portOutConnectionPolicy(PortIndex) const final;

    NodeType nodeType() const final { return NodeType::SUBTREE; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/subtree.svg");
    }

    QPushButton* collapseButton() { return _collapse_button; }

signals:
    void collapseButtonPushed();

private:

    QPushButton* _collapse_button;
    QString _base_ID;

};
*/


#endif // SUBTREE_NODEMODEL_HPP
