#include <QCommandLineParser>
#include <QApplication>
#include <nodes/NodeStyle>
#include <nodes/FlowViewStyle>
#include <nodes/ConnectionStyle>
#include <QDialog>

#include "models/ControlNodeModel.hpp"
#include "mainwindow.h"
#include "models/ActionNodeModel.hpp"
#include "models/RootNodeModel.hpp"

#include <nodes/DataModelRegistry>
#include "XML_utilities.hpp"
#include "startup_dialog.h"

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
    app.setApplicationName("BehaviorTreeEditor");


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

    parser.process( app );

    if( parser.isSet(test_option) )
    {
        MainWindow win( GraphicMode::EDITOR );
        win.setWindowTitle("Groot");
        win.show();
        win.loadFromXML( gTestXML );
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
            mode = dialog.getGraphicMode();
            if(dialog.exec() != QDialog::Accepted)
            {
                return 0;
            }
        }

        MainWindow win( mode );
        win.setWindowTitle("Groot");
        win.show();
        return app.exec();
    }
    return 0;
}
