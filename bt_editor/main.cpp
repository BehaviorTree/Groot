
#include <QApplication>
#include <nodes/NodeStyle>
#include <nodes/FlowViewStyle>
#include <nodes/ConnectionStyle>

#include "models/ControlNodeModel.hpp"
#include "mainwindow.h"
#include "models/ActionNodeModel.hpp"
#include "models/RootNodeModel.hpp"
#include "NodeFactory.hpp"

#include <nodes/DataModelRegistry>

#ifdef USING_ROS
#include <ros/ros.h>
#endif

using QtNodes::DataModelRegistry;
using QtNodes::FlowViewStyle;
using QtNodes::NodeStyle;
using QtNodes::ConnectionStyle;

int
main(int argc, char *argv[])
{
#ifdef USING_ROS
  ros::init(argc, argv, "behavior_tree_editor" );
#endif

  QApplication app(argc, argv);

 // EditorModel::loadMetaModelFromFile( "/home/dfaconti/ExampeEditorMetaModel.xml" );

  MainWindow win;
  win.show();

  return app.exec();
}
