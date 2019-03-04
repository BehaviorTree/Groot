#pragma once

#include <QObject>
#include <QLabel>
#include <QFile>
#include <QLineEdit>
#include <QFormLayout>
#include <QEvent>
#include <nodes/NodeDataModel>
#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <functional>
#include <QSvgRenderer>
#include "bt_editor/bt_editor_base.h"
#include "bt_editor/utils.h"

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDataModel;

class BehaviorTreeDataModel : public NodeDataModel
{
    Q_OBJECT

public:
    BehaviorTreeDataModel(const NodeModel &model );

    ~BehaviorTreeDataModel() override;

public:

    NodeType nodeType() const;

    virtual void setInstanceName(const QString& name);

public:

    void initWidget();

    virtual unsigned int nPorts(PortType portType) const override;

    ConnectionPolicy portOutConnectionPolicy(PortIndex) const final;

    NodeDataType dataType(PortType , PortIndex ) const final;

    std::shared_ptr<NodeData> outData(PortIndex port) final;

    void setInData(std::shared_ptr<NodeData>, int) final {}

    const QString &registrationName() const;

    const NodeModel &model() const { return _model; }

    QString name() const final { return registrationName(); }

    const QString& instanceName() const;

    PortsMapping getCurrentPortMapping() const;

    QWidget *embeddedWidget() final { return _main_widget; }

    QWidget *parametersWidget() { return _params_widget; }

    QJsonObject save() const override;

    void restore(QJsonObject const &) override;

    void lock(bool locked);

    void setPortMapping(const QString& port_name, const QString& value);

    int UID() const { return _uid; }

    bool eventFilter(QObject *obj, QEvent *event) override;


public slots:

    void updateNodeSize();

    void onHighlightPortValue(QString value);

protected:

    QFrame*  _main_widget;
    QFrame*  _params_widget;

    QLineEdit* _line_edit_name;

    std::map<QString, QWidget*> _ports_widgets;
    int16_t _uid;

    QFormLayout* _form_layout;
    QVBoxLayout* _main_layout;
    QLabel* _caption_label;
    QFrame* _caption_logo_left;
    QFrame* _caption_logo_right;

private:
    const NodeModel _model;
    QString _instance_name;
    QSvgRenderer* _icon_renderer;

    void readStyle();
    QString _style_icon;
    QColor  _style_caption_color;
    QString  _style_caption_alias;

signals:

    void parameterUpdated(QString, QWidget*);

    void instanceNameChanged();

    void portValueDoubleChicked(QLineEdit* value_port);

};


class GrootLineEdit: public QLineEdit
{
    Q_OBJECT
public:
    GrootLineEdit(QWidget* parent = nullptr): QLineEdit(parent) {}

    void mouseDoubleClickEvent(QMouseEvent *ev) override;
    void focusOutEvent(QFocusEvent* ev) override;
signals:
    void doubleClicked();
    void lostFocus();
};

