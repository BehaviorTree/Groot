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
    ControlNodeModel(const QString& ID, const ParameterWidgetCreators &parameters);

    virtual ~ControlNodeModel() override = default;

    unsigned int  nPorts(PortType) const final
    { return 1; }

    virtual NodeType nodeType() const final { return NodeType::CONTROL; }

    virtual QSvgRenderer* icon() const override { return _renderer; }

protected:

    QSvgRenderer* _renderer;
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
    virtual ~SequenceModel() = default;

    static const char* Name() { return ("Sequence"); }
};


class FallbackModel: public  ControlNodeModelBase<FallbackModel>
{
public:
    FallbackModel();
    virtual ~FallbackModel()  = default;
    static const char* Name() { return ("Fallback"); }
};


class SequenceStarModel: public ControlNodeModelBase<SequenceStarModel>
{
public:
    SequenceStarModel();
    virtual ~SequenceStarModel()  = default;
    static const char* Name() { return ("SequenceStar"); }

};



class IfThenElseModel: public  ControlNodeModelBase<IfThenElseModel>
{
public:
    IfThenElseModel();
    virtual ~IfThenElseModel()  = default;
    static const char* Name() { return ("IfThenElse"); }
};



