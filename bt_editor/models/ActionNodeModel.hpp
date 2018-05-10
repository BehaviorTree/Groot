#pragma once

#include "BehaviorTreeNodeModel.hpp"


class ActionNodeModel : public BehaviorTreeNodeModel
{
public:
    ActionNodeModel(const QString& action_ID);

    virtual ~ActionNodeModel()  = default;

    virtual unsigned int  nPorts(PortType portType) const override;

    virtual void restore(QJsonObject const &) override final;

private:

    void init();

    virtual void setInstanceName(const QString& name) override;
};

