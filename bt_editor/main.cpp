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

    qRegisterMetaType<AbsBehaviorTree>();

    QCommandLineParser parser;
    parser.setApplicationDescription("Groot. The fancy BehaviorTree Editor");
    parser.addHelpOption();

    QCommandLineOption test_option(QStringList() << "t" << "test",
                                   QCoreApplication::translate("main", "Load dummy"));
    parser.addOption(test_option);
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
        StartupDialog dialog;
        dialog.setWindowFlags( Qt::FramelessWindowHint );

        if( dialog.exec() == QDialog::Accepted)
        {
            MainWindow win( dialog.getGraphicMode() );
            win.setWindowTitle("Groot");
            win.show();
            return app.exec();
        }
    }
    return 0;
}
