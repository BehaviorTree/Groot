#ifndef SUBTREE_NODEMODEL_HPP
#define SUBTREE_NODEMODEL_HPP

#include "BehaviorTreeNodeModel.hpp"

class SubtreeNodeModel : public BehaviorTreeNodeModel
{
public:
    SubtreeNodeModel(QString subtree_ID,
                     const ParameterWidgetCreators &creators);

    virtual ~SubtreeNodeModel() = default;

    virtual unsigned int  nPorts(PortType portType) const override;

    virtual QString name() const override { return QString("SubTree"); }
};


#endif // SUBTREE_NODEMODEL_HPP
