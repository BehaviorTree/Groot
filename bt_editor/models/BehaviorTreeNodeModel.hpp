#pragma once

#include <QObject>
#include <QLabel>
#include <QLineEdit>
#include <QFormLayout>

#include <nodes/NodeDataModel>
#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <functional>
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
  BehaviorTreeDataModel(const QString &label_name,
                        const QString &instance_name,
                        const ParameterWidgetCreators &parameters );

  ~BehaviorTreeDataModel() override = default;

public:

  NodeDataType dataType(PortType , PortIndex ) const final;

  std::shared_ptr<NodeData> outData(PortIndex port) final;

  void setInData(std::shared_ptr<NodeData>, int) final {}

  const QString& registrationName() const;

  QString name() const final { return registrationName(); }

  const QString& instanceName() const;

  virtual std::pair<QString,QColor> caption() const = 0;

  std::vector<std::pair<QString, QString> > getCurrentParameters() const;

  QWidget *embeddedWidget() final { return _main_widget; }

  QWidget *parametersWidget() { return _params_widget; }

  QJsonObject save() const final;

  void restore(QJsonObject const &) final;

  void lock(bool locked);

  void setParameterValue(const QString& label, const QString& value);

  virtual const char* className() const = 0;

  int UID() const { return _uid; }

  virtual NodeType nodeType() const = 0;

public slots:

  void updateNodeSize();

  void setInstanceName(const QString& name);

protected:
  QFrame*  _main_widget;
  QFrame*  _params_widget;

  QLineEdit* _line_edit_name;

  std::map<QString, QWidget*> _params_map;
  int16_t _uid;

  QFormLayout* _form_layout;
  QVBoxLayout* _main_layout;

private:
  const QString _registration_name;
  QString _instance_name;


signals:

  void parameterUpdated(QString, QWidget*);

  void instanceNameChanged();

};
