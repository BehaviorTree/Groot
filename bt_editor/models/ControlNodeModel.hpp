#pragma once

#include <QtCore/QObject>
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QPixmap>
#include <nodes/NodeDataModel>
#include <iostream>
#include <memory>
#include <QXmlStreamWriter>
#include <QSvgRenderer>
#include "BehaviorTreeNodeModel.hpp"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDataModel;
/*
class ControlNodeModel : public BehaviorTreeDataModel
{

public:
    ControlNodeModel(const BT_NodeModel &model);

    virtual ~ControlNodeModel() override = default;

    unsigned int  nPorts(PortType) const final
    { return 1; }

    virtual NodeType nodeType() const final { return NodeType::CONTROL; }

};

//-------------------------------------------------
class SequenceModel: public ControlNodeModel
{
public:
    SequenceModel();

    static const char* Name() { return ("Sequence"); }

    static TreeNodeModel NodeModel(){
        return { Name(), NodeType::CONTROL, {} };
    }

    std::pair<QString,QColor> caption() const override { return {"Sequence","#ff99bb"}; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/sequence.svg");
    }
};


class FallbackModel: public  ControlNodeModel
{
public:
    FallbackModel();

    static const char* Name() { return ("Fallback"); }

    static TreeNodeModel NodeModel()
    {
        return { Name(), NodeType::CONTROL, {} };
    }

    std::pair<QString,QColor> caption() const override { return { Name(),"#77ccff"}; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/fallback.svg");
    }
};


class SequenceStarModel: public ControlNodeModel
{
public:
    SequenceStarModel();

    static const char* Name() { return ("SequenceStar"); }

    static TreeNodeModel NodeModel()
    {
        return { Name(), NodeType::CONTROL, {} };
    }

    std::pair<QString,QColor> caption() const override { return { Name(),"#ff5af3"}; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/sequence_star.svg");
    }

};

class FallbackStarModel: public  ControlNodeModel
{
public:
    FallbackStarModel();

    static const char* Name() { return ("FallbackStar"); }

    static TreeNodeModel NodeModel()
    {
        return { Name(), NodeType::CONTROL, {} };
    }

    std::pair<QString,QColor> caption() const override { return { Name(),"#44aadd"}; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/fallback_star.svg");
    }
};


class IfThenElseModel: public  ControlNodeModel
{
public:
    IfThenElseModel();

    virtual ~IfThenElseModel()  = default;

    static const char* Name() { return ("IfThenElse"); }

    static TreeNodeModel NodeModel()
    {
        return { Name(), NodeType::CONTROL, {} };
    }
};
*/


