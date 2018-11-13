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
    QString file_xml = readFile(":/crossdor_with_subtree.xml");

    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );
    QString saved_xml = main_win->saveToXML();

    QVERIFY2( file_xml.simplified() == saved_xml.simplified(),
              "Loaded and saved XMl are not the same" );

    QApplication::processEvents();
    QTest::qSleep ( 1000 );
    //-------------------------------
    // Compare AbsBehaviorTree
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );
    auto tree_A1 = BuildTreeFromScene( main_win->getTabByName("MainTree")->scene() );
    auto tree_A2 = BuildTreeFromScene( main_win->getTabByName("DoorClosed")->scene() );

    main_win->loadFromXML( saved_xml );
    auto tree_B1 = BuildTreeFromScene( main_win->getTabByName("MainTree")->scene() );
    auto tree_B2 = BuildTreeFromScene( main_win->getTabByName("DoorClosed")->scene() );

    bool same_maintree   = tree_A1 == tree_B1;
    bool same_doorclosed = tree_A2 == tree_B2;
    if( !same_maintree )
    {
        tree_A1.debugPrint();
        tree_B1.debugPrint();
    }
    if( !same_doorclosed )
    {
        tree_A2.debugPrint();
        tree_B2.debugPrint();
    }

    QVERIFY2( same_maintree, "AbsBehaviorTree comparison fails" );
    QVERIFY2( same_doorclosed, "AbsBehaviorTree comparison fails" );

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

    //---------------------------------
    // Expanded tree should save the same file
    QString saved_xml_expanded = main_win->saveToXML();
    QVERIFY2( saved_xml_expanded.simplified() == saved_xml.simplified(),
              "Loaded and saved XMl are not the same" );

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
