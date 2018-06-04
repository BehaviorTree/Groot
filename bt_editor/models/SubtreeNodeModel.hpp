#ifndef SUBTREE_NODEMODEL_HPP
#define SUBTREE_NODEMODEL_HPP

#include "BehaviorTreeNodeModel.hpp"

class SubtreeNodeModel : public BehaviorTreeDataModel
{
public:
    SubtreeNodeModel(const QString& subtree_ID,
                     const ParameterWidgetCreators &creators);

    ~SubtreeNodeModel() = default;

    virtual unsigned int  nPorts(PortType portType) const override;

    virtual const char* className() const override final
    {
      return "SubTree";
    }

   QString caption() const override {
        return "SubTree";
    }

    virtual NodeType nodeType() const override final { return NodeType::SUBTREE; }
};


#endif // SUBTREE_NODEMODEL_HPP
