#pragma once

#include "BehaviorTreeNodeModel.hpp"


class ActionNodeModel : public BehaviorTreeNodeModel
{
public:
    ActionNodeModel(const QString& action_ID);

    virtual ~ActionNodeModel()  = default;

    virtual unsigned int  nPorts(PortType portType) const override;

    virtual QString name() const override
    {
      return name_;
    }
private:
    const QString name_;
};

