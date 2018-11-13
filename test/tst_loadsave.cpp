#include <QtTest>
#include <QGuiApplication>


// add necessary includes here
#include "bt_editor/mainwindow.h"
#include "bt_editor/utils.h"
#include "bt_editor/models/SubtreeNodeModel.hpp"

QString readFile(const char* name)
{
    QString fileName(name);
    QFile file(fileName);
    bool ret = file.open(QIODevice::ReadOnly);
    QString data = file.readAll();
    file.close();
    return data;
}

class GrootTest : public QObject
{
    Q_OBJECT

public:
    GrootTest();
    ~GrootTest();

private slots:
    void initTestCase();
    void cleanupTestCase();
    void loadFile();

private:
    MainWindow *main_win;
};

GrootTest::GrootTest()
{

}

GrootTest::~GrootTest()
{

}

void GrootTest::initTestCase()
{
    main_win = new MainWindow(GraphicMode::EDITOR, nullptr);
    main_win->resize(1200, 800);
    main_win->show();
}

void GrootTest::cleanupTestCase()
{
    QApplication::processEvents();
    main_win->on_actionClear_triggered();
    QTest::qSleep ( 1000 );
    main_win->close();
}

void GrootTest::loadFile()
{
    QString xml = readFile(":/crossdor_with_subtree.xml");

    main_win->on_actionClear_triggered();
    main_win->loadFromXML( xml );
   // QString saved_xml = main_win->saveToXML();

   // std::cout << saved_xml.toStdString() << std::endl;

//    QVERIFY2( xml.simplified() == saved_xml.simplified(),
//              "Loaded and saved XMl are not the same" );

    QApplication::processEvents();
    QTest::qSleep ( 1000 );

    //-------------------------------
    // Next: expand the Subtree [DoorClosed]
    auto tree = BuildTreeFromScene( main_win->getTabByName("MainTree")->scene() );

    auto subtree_abs_node = tree.findNode("DoorClosed");
    QVERIFY2(subtree_abs_node, "Can't find node with ID [DoorClosed]");

    {
        auto data_model = subtree_abs_node->corresponding_node->nodeDataModel();
        auto subtree_model = dynamic_cast<SubtreeNodeModel*>(data_model);
        QVERIFY2(subtree_model, "Node [DoorClosed] is not SubtreeNodeModel");
        QTest::mouseClick( subtree_model->expandButton(), Qt::LeftButton );
    }
    QApplication::processEvents();
    QTest::qSleep ( 1000 );

    //-------------------------------
    // Next: collapse again Subtree [DoorClosed]
    tree = BuildTreeFromScene( main_win->getTabByName("MainTree")->scene() );

    subtree_abs_node = tree.findNode("DoorClosed");
    QVERIFY2(subtree_abs_node, "Can't find node with ID [DoorClosed]");

    {
        auto data_model = subtree_abs_node->corresponding_node->nodeDataModel();
        auto subtree_model = dynamic_cast<SubtreeNodeModel*>(data_model);
        QVERIFY2(subtree_model, "Node [DoorClosed] is not SubtreeNodeModel");
        QTest::mouseClick( subtree_model->expandButton(), Qt::LeftButton );
    }

    QApplication::processEvents();
    QTest::qSleep ( 1000 );

}

QTEST_MAIN(GrootTest)

#include "tst_loadsave.moc"
