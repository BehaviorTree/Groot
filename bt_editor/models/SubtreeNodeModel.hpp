#ifndef SUBTREE_NODEMODEL_HPP
#define SUBTREE_NODEMODEL_HPP

#include "BehaviorTreeNodeModel.hpp"

class SubtreeNodeModel : public BehaviorTreeNodeModel
{
public:
    SubtreeNodeModel(const QString& subtree_ID,
                     const ParameterWidgetCreators &creators);

    virtual ~SubtreeNodeModel() = default;

    virtual unsigned int  nPorts(PortType portType) const override;

};


#endif // SUBTREE_NODEMODEL_HPP
