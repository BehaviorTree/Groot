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
#include <memory.hpp>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDataModel;

class ControlNodeModel : public BehaviorTreeDataModel
{

public:
    ControlNodeModel(const QString& ID, const ParameterWidgetCreators &parameters);

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

    virtual const char* className() const final
    {
        return T::Name();
    }
};

//-------------------------------------------------
template<typename T> inline
ControlNodeModelBase<T>::ControlNodeModelBase():
    ControlNodeModel(T::Name(), ParameterWidgetCreators() )
{
    _main_widget->setToolTip( T::Name() );
}




//-------------------------------------------------
class SequenceModel: public ControlNodeModelBase<SequenceModel>
{
public:
    SequenceModel();
    static const char* Name() { return ("Sequence"); }

    std::pair<QString,QColor> caption() const override { return {"Sequence","#ff6699"}; }

    virtual QSvgRenderer* icon() const override {
        static QSvgRenderer r(tr(":/icons/svg/arrow_right.svg"));
        return &r;
    }
};


class FallbackModel: public  ControlNodeModelBase<FallbackModel>
{
public:
    FallbackModel();
    static const char* Name() { return ("Fallback"); }

    std::pair<QString,QColor> caption() const override { return {"Fallback","#33adff"}; }

    virtual QSvgRenderer* icon() const override {
        static QSvgRenderer r(tr(":/icons/svg/question_mark.svg"));
        return &r;
    }
};


class SequenceStarModel: public ControlNodeModelBase<SequenceStarModel>
{
public:
    SequenceStarModel();
    static const char* Name() { return ("SequenceStar"); }

    std::pair<QString,QColor> caption() const override { return {"SequenceStar","#ff1ab3"}; }

    virtual QSvgRenderer* icon() const override {
        static QSvgRenderer r(tr(":/icons/svg/arrow_right.svg"));
        return &r;
    }
};



class IfThenElseModel: public  ControlNodeModelBase<IfThenElseModel>
{
public:
    IfThenElseModel();
    virtual ~IfThenElseModel()  = default;
    static const char* Name() { return ("IfThenElse"); }
};



