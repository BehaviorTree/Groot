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

    unsigned int  nPorts(PortType) const override final
    { return 1; }

    virtual NodeType nodeType() const override final { return NodeType::CONTROL; }

private:

};
//------------------------------------------------

template <typename T>
class ControlNodeModelBase : public ControlNodeModel
{
public:
    ControlNodeModelBase();
    virtual ~ControlNodeModelBase() override = default;

    virtual const char* className() const override final
    {
        return T::Name();
    }

protected:

    void setLabelImage(QString pixmap_address);
};

//-------------------------------------------------
template<typename T> inline
ControlNodeModelBase<T>::ControlNodeModelBase():
    ControlNodeModel(T::Name(), ParameterWidgetCreators() )
{
    _main_widget->setToolTip( T::Name() );
}

template<typename T> inline
void ControlNodeModelBase<T>::setLabelImage(QString pixmap_address)
{
    QPixmap pix;
    if( pix.load(pixmap_address))
    {
        _label_ID->setPixmap(pix);
        _label_ID->setFixedSize( QSize(30,30) );
        _label_ID->setScaledContents(true);
    }
    else{
        _label_ID->setText( name() );
    }
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



