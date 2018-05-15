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

class ControlNodeModel : public BehaviorTreeNodeModel
{

public:
    ControlNodeModel(const QString& ID, const ParameterWidgetCreators &parameters);

    virtual ~ControlNodeModel() = default;

    unsigned int  nPorts(PortType) const override final
    { return 1; }


    virtual void setInstanceName(const QString &name) override;

//    virtual void restore(QJsonObject const &modelJson) override final
//    {
//      setInstanceName( modelJson["alias"].toString() );
//    }
private:
    void init();
};
//------------------------------------------------

template <typename T>
class ControlNodeModelBase : public ControlNodeModel
{
public:
    ControlNodeModelBase();
    virtual ~ControlNodeModelBase() {}

    virtual const char* className() const override final
    {
      return T::staticName();
    }

protected:

    void setLabelImage(QString pixmap_address);
};

//-------------------------------------------------
template<typename T> inline
ControlNodeModelBase<T>::ControlNodeModelBase():
  ControlNodeModel(T::staticName(), ParameterWidgetCreators() )
{
    _main_widget->setToolTip( T::staticName() );
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

    static const char* staticName() { return ("Sequence"); }
};


class FallbackModel: public  ControlNodeModelBase<FallbackModel>
{
public:
    FallbackModel();
    virtual ~FallbackModel()  = default;
    static const char* staticName() { return ("Fallback"); }
};


class SequenceStarModel: public ControlNodeModelBase<SequenceStarModel>
{
public:
    SequenceStarModel();
    virtual ~SequenceStarModel()  = default;
    static const char* staticName() { return ("SequenceStar"); }

};



class IfThenElseModel: public  ControlNodeModelBase<IfThenElseModel>
{
public:
    IfThenElseModel();
    virtual ~IfThenElseModel()  = default;
    static const char* staticName() { return ("IfThenElse"); }
};



