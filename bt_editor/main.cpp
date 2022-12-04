#include <QCommandLineParser>
#include <QApplication>
#include <QDialog>
#include <nodes/NodeStyle>
#include <nodes/FlowViewStyle>
#include <nodes/ConnectionStyle>
#include <nodes/DataModelRegistry>

#include "mainwindow.h"
#include "XML_utilities.hpp"
#include "models/RootNodeModel.hpp"

using QtNodes::DataModelRegistry;
using QtNodes::FlowViewStyle;
using QtNodes::NodeStyle;
using QtNodes::ConnectionStyle;

int
main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Groot");
    app.setWindowIcon(QPixmap(":/icons/BT.png"));
    app.setOrganizationName("EurecatRobotics");
    app.setOrganizationDomain("eurecat.org");

    qRegisterMetaType<AbsBehaviorTree>();

    QCommandLineParser parser;
    parser.setApplicationDescription("Groot. The fancy BehaviorTree Editor");
    parser.addHelpOption();

    QCommandLineOption test_option(QStringList() << "t" << "test",
                                   "Load dummy data");
    parser.addOption(test_option);

    parser.process( app );

    QFile styleFile( ":/stylesheet.qss" );
    styleFile.open( QFile::ReadOnly );
    QString style( styleFile.readAll() );
    app.setStyleSheet( style );

    if( parser.isSet(test_option) )
    {
        MainWindow win( GraphicMode::EDITOR );
        win.setWindowTitle("Groot");
        win.show();
        win.loadFromXML( ":/crossdoor_with_subtree.xml" );
        return app.exec();
    }
    else{
        auto mode = GraphicMode::EDITOR;

        MainWindow win( mode );
        win.show();
        return app.exec();
    }
}
