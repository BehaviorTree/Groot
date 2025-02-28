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

    QCommandLineOption file_option(QStringList() << "file",
                                   "Load a file (only in editor mode)",
                                   "tree.xml");
    parser.addOption(file_option);

    QCommandLineOption output_svg_option(QStringList() << "output-svg",
                                         "Save the input file to an svg",
                                         "output.svg");
    parser.addOption(output_svg_option);

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
                return 1;
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

        if( parser.isSet(file_option) )
        {
            if ( mode != GraphicMode::EDITOR )
            {
                std::cout << "--file can only be passed in editor mode" << std::endl;
                return 1;
            }

            QString fileName = parser.value(file_option);
            std::cout << "Loading file: " << fileName.toStdString() << std::endl;

            // Open file
            QFile file(fileName);
            if (!file.open(QIODevice::ReadOnly))
            {
                std::cout << "Cannot open file" << std::endl;
                return 1;
            }

            // Read file to xml
            QString xml_text;
            QTextStream in(&file);
            while (!in.atEnd()) {
                xml_text += in.readLine();
            }

            // Show xml
            win.loadFromXML( xml_text );
        }


        if( parser.isSet(output_svg_option) )
        {
            if ( !parser.isSet(file_option))
            {
                std::cout << "--output-svg needs the --file" << std::endl;
                return 1;
            }
            QString svgFile = parser.value(output_svg_option);

            std::cout << "Writing to: " << svgFile.toStdString() << std::endl;
            win.currentTabInfo()->saveSvgFile(svgFile);
            return 0;
        }

        win.show();
        return app.exec();
    }
}
