#include <QCommandLineParser>
#include <QApplication>
#include <QDialog>
#include <nodes/NodeStyle>
#include <nodes/FlowViewStyle>
#include <nodes/ConnectionStyle>
#include <nodes/DataModelRegistry>

#include "mainwindow.h"
#include "XML_utilities.hpp"
#include "startup_dialog.h"
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

    QCommandLineOption mode_option(QStringList() << "mode",
                                   "Start in one of these modes: [editor,monitor,replay]",
                                   "mode");
    parser.addOption(mode_option);

    QCommandLineOption address_option(QStringList() << "address",
                                      "Address to connect to (defaults to localhost)",
                                      "address");
    parser.addOption(address_option);
    QCommandLineOption pub_port_option(QStringList() << "publisher_port",
                                       "Publisher port number (defaults to 1666)",
                                       "publisher_port");
    parser.addOption(pub_port_option);
    QCommandLineOption srv_port_option(QStringList() << "server_port",
                                       "Server port number (defaults to 1667)",
                                       "server_port");
    parser.addOption(srv_port_option);
    QCommandLineOption autoconnect_option(QStringList() << "autoconnect",
                                          "Autoconnect to monitor");
    parser.addOption(autoconnect_option);

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

        if( parser.isSet(mode_option) )
        {
            QString opt_mode = parser.value(mode_option);
            if( opt_mode == "editor")
            {
                mode = GraphicMode::EDITOR;
            }
            else if( opt_mode == "monitor")
            {
                mode = GraphicMode::MONITOR;
            }
            else if( opt_mode == "replay")
            {
                mode = GraphicMode::REPLAY;
            }
            else{
                std::cout << "wrong mode passed to --mode. Use on of these: editor / monitor /replay"
                          << std::endl;
                return 0;
            }
        }
        else{
            StartupDialog dialog;
            dialog.setWindowFlags( Qt::FramelessWindowHint );
            if(dialog.exec() != QDialog::Accepted)
            {
                return 0;
            }
            mode = dialog.getGraphicMode();
        }

        // Get the monitor options.
        const QString monitor_address = parser.value(address_option);
        const QString monitor_pub_port = parser.value(pub_port_option);
        const QString monitor_srv_port = parser.value(srv_port_option);
        const bool monitor_autoconnect = parser.isSet(autoconnect_option);

        // Start the main application.
        MainWindow win( mode, monitor_address, monitor_pub_port,
                        monitor_srv_port, monitor_autoconnect );
        win.show();
        return app.exec();
    }
}
