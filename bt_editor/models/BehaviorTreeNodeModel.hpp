#pragma once

#include <QObject>
#include <QLabel>
#include <QFormLayout>
#include <QComboBox>

#include <nodes/NodeDataModel>
#include <iostream>
#include <memory>
#include <vector>
#include <map>
#include <functional>

using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDataModel;

struct ParameterWidgetCreator{
    QString label;
    std::function<QWidget*()> instance_factory;
};

typedef std::vector<ParameterWidgetCreator> ParameterWidgetCreators;


class BehaviorTreeNodeModel : public NodeDataModel
{
    Q_OBJECT

public:
  BehaviorTreeNodeModel(const QString &label_name,
                        const QString &instance_name,
                        const ParameterWidgetCreators &parameters );

  virtual ~BehaviorTreeNodeModel() {}

  virtual bool captionVisible() const override final
  { return true; }

public:

  NodeDataType dataType(PortType , PortIndex ) const override final;

  virtual std::shared_ptr<NodeData> outData(PortIndex port) override final;

  void setInData(std::shared_ptr<NodeData>, int) override final {}

  virtual QString caption() const override;

  const QString& registrationName() const;

  QString name() const override final { return registrationName(); }

  const QString& instanceName() const;
  virtual void setInstanceName(const QString& name);

  std::vector<std::pair<QString, QString> > getCurrentParameters() const;

  virtual QWidget *embeddedWidget() override final { return _main_widget; }

  virtual QWidget *parametersWidget() { return _params_widget; }

  virtual QJsonObject save() const override final;

  virtual void restore(QJsonObject const &) override final;

  virtual void lock(bool locked) final;

  void setParameterValue(const QString& label, const QString& value);

  virtual const char* className() const = 0;


protected:
  QWidget*   _main_widget;
  QWidget*   _params_widget;
  std::map<QString,QWidget*>   _params_map;
  QFormLayout *_form_layout;
  QLabel*    _label_ID;
  QLineEdit* _line_edit_name;
private:
  const QString _registration_name;
  QString _instance_name;

signals:

  void adjustSize();

  void parameterUpdated(QString, QWidget*);

  void instanceNameChanged();
};
