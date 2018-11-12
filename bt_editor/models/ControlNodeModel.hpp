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

class ControlNodeModel : public BehaviorTreeDataModel
{

public:
    ControlNodeModel(const QString& ID, const TreeNodeModel &model);

    virtual ~ControlNodeModel() override = default;

    unsigned int  nPorts(PortType) const final
    { return 1; }

    virtual NodeType nodeType() const final { return NodeType::CONTROL; }

};
//------------------------------------------------

template <typename T>
class ControlNodeModelBase : public ControlNodeModel
{
public:
    ControlNodeModelBase();
    virtual ~ControlNodeModelBase() override = default;
};

//-------------------------------------------------
template<typename T> inline
ControlNodeModelBase<T>::ControlNodeModelBase():
    ControlNodeModel(T::Name(), TreeNodeModel() )
{
    _main_widget->setToolTip( T::Name() );
}




//-------------------------------------------------
class SequenceModel: public ControlNodeModelBase<SequenceModel>
{
public:
    SequenceModel();
    static const char* Name() { return ("Sequence"); }

    std::pair<QString,QColor> caption() const override { return {"Sequence","#ff99bb"}; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/sequence.svg");
    }
};


class FallbackModel: public  ControlNodeModelBase<FallbackModel>
{
public:
    FallbackModel();
    static const char* Name() { return ("Fallback"); }

    std::pair<QString,QColor> caption() const override { return { Name(),"#77ccff"}; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/fallback.svg");
    }
};


class SequenceStarModel: public ControlNodeModelBase<SequenceStarModel>
{
public:
    SequenceStarModel();
    static const char* Name() { return ("SequenceStar"); }

    std::pair<QString,QColor> caption() const override { return { Name(),"#ff5af3"}; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/sequence_star.svg");
    }

};

class FallbackStarModel: public  ControlNodeModelBase<FallbackStarModel>
{
public:
    FallbackStarModel();
    static const char* Name() { return ("FallbackStar"); }

    std::pair<QString,QColor> caption() const override { return { Name(),"#44aadd"}; }

    virtual QString captionIicon() const override {
        return(":/icons/svg/fallback_star.svg");
    }
};


class IfThenElseModel: public  ControlNodeModelBase<IfThenElseModel>
{
public:
    IfThenElseModel();
    virtual ~IfThenElseModel()  = default;
    static const char* Name() { return ("IfThenElse"); }
};



