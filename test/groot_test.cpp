#include <QtTest>
#include <QGuiApplication>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QApplication>

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

void TestMouseEvent(QGraphicsView* view, QEvent::Type type, QPoint pos, Qt::MouseButton button, Qt::KeyboardModifier modifier = Qt::NoModifier)
{
    auto event = new QMouseEvent(type, pos, view->viewport()->mapToGlobal(pos), button, button, modifier);

    QApplication::postEvent(view->viewport(), event);
    QApplication::processEvents();
}
void SleepAndRefresh(int ms)
{
    QApplication::processEvents();
    QTest::qSleep ( ms );
    QApplication::processEvents();
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
    void undoRedo();

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
    main_win->close();
}

void GrootTest::loadFile()
{
    QString file_xml = readFile(":/crossdor_with_subtree.xml");

    main_win->on_actionClear_triggered();
    main_win->clearTreeModels();
    main_win->loadFromXML( file_xml );
    QString saved_xml = main_win->saveToXML();

    QVERIFY2( file_xml.simplified() == saved_xml.simplified(),
              "Loaded and saved XMl are not the same" );

    SleepAndRefresh( 500 );
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
    if( !same_maintree )
    {
        tree_A1.debugPrint();
        tree_B1.debugPrint();
    }
    QVERIFY2( same_maintree, "AbsBehaviorTree comparison fails" );

    bool same_doorclosed = tree_A2 == tree_B2;
    if( !same_doorclosed )
    {
        tree_A2.debugPrint();
        tree_B2.debugPrint();
    }

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
    SleepAndRefresh( 500 );

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

    SleepAndRefresh( 500 );
}





void GrootTest::undoRedo()
{
    QString file_xml = readFile(":/show_all.xml");
    main_win->on_actionClear_triggered();
    main_win->clearTreeModels();
    main_win->loadFromXML( file_xml );

    //------------------------------------------
    AbsBehaviorTree abs_tree_A, abs_tree_B;
    {
        auto container = main_win->currentTabInfo();
        auto view = container->view();
        abs_tree_A = BuildTreeFromScene( container->scene() );

        QApplication::processEvents();
        SleepAndRefresh( 500 );

        auto pippo_node = abs_tree_A.findNode("Pippo");
        auto gui_node = pippo_node->corresponding_node;
        QPoint pos = view->mapFromScene(pippo_node->pos);
        QPoint pos_offset = QPoint(100,0);

        auto old_pos = view->mapFromScene( container->scene()->getNodePosition( *gui_node ) );

        QTest::mouseClick( view->viewport(), Qt::LeftButton, Qt::NoModifier, pos );

        QVERIFY2( gui_node->nodeGraphicsObject().isSelected(), "Pippo is not selected");

        TestMouseEvent(view, QEvent::MouseButtonPress,    pos , Qt::LeftButton);
        TestMouseEvent(view, QEvent::MouseMove,           pos + pos_offset, Qt::LeftButton);
        TestMouseEvent(view, QEvent::MouseButtonRelease,  pos + pos_offset, Qt::LeftButton);

        QPoint new_pos = view->mapFromScene( container->scene()->getNodePosition( *gui_node ) );

        QCOMPARE( old_pos + pos_offset, new_pos);
        SleepAndRefresh( 500 );
    }
    //---------- test undo ----------
    {
        main_win->onUndoInvoked();
        SleepAndRefresh( 500 );

        auto container = main_win->currentTabInfo();
        abs_tree_B = BuildTreeFromScene( container->scene() );
    }

    QCOMPARE( abs_tree_A, abs_tree_B);
    SleepAndRefresh( 1000 );

    {
        main_win->onUndoInvoked();
        SleepAndRefresh( 1000 );
        auto container = main_win->currentTabInfo();
        auto empty_abs_tree = BuildTreeFromScene( container->scene() );
        QCOMPARE( empty_abs_tree.nodesCount(), 0);

        main_win->onUndoInvoked();
        main_win->onUndoInvoked();
        main_win->onUndoInvoked();
    }
}

QTEST_MAIN(GrootTest)

#include "groot_test.moc"
