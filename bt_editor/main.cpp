#include <QCommandLineParser>
#include <QApplication>
#include <nodes/NodeStyle>
#include <nodes/FlowViewStyle>
#include <nodes/ConnectionStyle>

#include "models/ControlNodeModel.hpp"
#include "mainwindow.h"
#include "models/ActionNodeModel.hpp"
#include "models/RootNodeModel.hpp"

#include <nodes/DataModelRegistry>
#include "XML_utilities.hpp"

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
  QApplication app(argc, argv);
  app.setApplicationName("BehaviorTreeEditor");

  QCommandLineParser parser;
  parser.setApplicationDescription("BehaviorTreeEditor: just a fancy XML editor");
  parser.addHelpOption();

  QCommandLineOption test_option(QStringList() << "t" << "test",
                                 QCoreApplication::translate("main", "Load dummy"));
  parser.addOption(test_option);
  parser.process( app );

  MainWindow win;

  win.show();

  if( parser.isSet(test_option) )
  {
    win.loadFromXML( gTestXML );
  }


  return app.exec();
}
