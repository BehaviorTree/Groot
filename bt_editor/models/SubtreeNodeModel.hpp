#ifndef SUBTREE_NODEMODEL_HPP
#define SUBTREE_NODEMODEL_HPP

#include "BehaviorTreeNodeModel.hpp"
#include <QPushButton>

class SubtreeNodeModel : public BehaviorTreeDataModel
{
    Q_OBJECT
public:

    SubtreeNodeModel(const NodeModel& model);

    ~SubtreeNodeModel() override = default;

    void setExpanded(bool expand);

    bool expanded() const { return _expanded; }

    unsigned int  nPorts(PortType portType) const override
    {
        int out_port = _expanded ? 1 : 0;
        return portType == PortType::In ? 1:out_port;
    }

    virtual const char* className() const final { return Name(); }

    static const char* Name() { return "SubTree";  }

    QPushButton* expandButton() { return _expand_button; }

    virtual void setInstanceName(const QString& name) override;

    QJsonObject save() const override;

    void restore(QJsonObject const &) override;

signals:
    void expandButtonPushed();

private:
    QPushButton* _expand_button;
    bool _expanded;

};


#endif // SUBTREE_NODEMODEL_HPP
