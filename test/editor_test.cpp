#include "groot_test_base.h"
#include "bt_editor/sidepanel_editor.h"
#include <QAction>
#include <QLineEdit>

class EditorTest : public GrootTestBase
{
    Q_OBJECT

public:
    EditorTest() {}
    ~EditorTest() {}

private slots:
    void initTestCase();
    void cleanupTestCase();
    void renameTabs();
    void loadFile();
    void loadFailed();
    void savedFileSameAsOriginal();
    void undoRedo();
    void testSubtree();
    void modifyCustomModel();
    void multipleSubtrees();
    void editText();
    void loadModelLess();
    void longNames();
    void clearModels();
    void undoWithSubtreeExpanded();
};


void EditorTest::initTestCase()
{
    main_win = new MainWindow(GraphicMode::EDITOR, nullptr);
    main_win->resize(1200, 800);
    main_win->show();
}

void EditorTest::cleanupTestCase()
{
    QApplication::processEvents();
    sleepAndRefresh( 1000 );
    main_win->on_actionClear_triggered();
    main_win->close();
}

void EditorTest::loadFile()
{
    QString file_xml = readFile(":/crossdoor_with_subtree.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    QString saved_xml = main_win->saveToXML();

    QFile qFile("crossdoor_EditorTest_loadFile.xml");
    if (qFile.open(QIODevice::WriteOnly))
    {
        QTextStream out(&qFile); out << saved_xml;
        qFile.close();
    }

    sleepAndRefresh( 500 );
    //-------------------------------
    // Compare AbsBehaviorTree
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );
    auto tree_A1 = getAbstractTree("MainTree");
    auto tree_A2 = getAbstractTree("DoorClosed");

    main_win->loadFromXML( saved_xml );
    auto tree_B1 = getAbstractTree("MainTree");
    auto tree_B2 = getAbstractTree("DoorClosed");

    bool same_maintree   = (tree_A1 == tree_B1);
    if( !same_maintree )
    {
        tree_A1.debugPrint();
        tree_B1.debugPrint();
    }
    QVERIFY2( same_maintree, "AbsBehaviorTree comparison fails" );

    QVERIFY2( file_xml.simplified() == saved_xml.simplified(),
             "Loaded and saved XML are not the same" );


    bool same_doorclosed = tree_A2 == tree_B2;
    if( !same_doorclosed )
    {
        tree_A2.debugPrint();
        tree_B2.debugPrint();
    }

    QVERIFY2( same_doorclosed, "AbsBehaviorTree comparison fails" );

    //-------------------------------
    // Next: expand the Subtree [DoorClosed]
    auto tree = getAbstractTree("MainTree");

    auto subtree_abs_node = tree.findFirstNode("DoorClosed");
    QVERIFY2(subtree_abs_node, "Can't find node with ID [DoorClosed]");

    {
        auto data_model = subtree_abs_node->graphic_node->nodeDataModel();
        auto subtree_model = dynamic_cast<SubtreeNodeModel*>(data_model);
        QVERIFY2(subtree_model, "Node [DoorClosed] is not SubtreeNodeModel");
        QTest::mouseClick( subtree_model->expandButton(), Qt::LeftButton );
    }
    sleepAndRefresh( 500 );

    //---------------------------------
    // Expanded tree should save the same file
    QString saved_xml_expanded = main_win->saveToXML();
    QVERIFY2( saved_xml_expanded.simplified() == saved_xml.simplified(),
              "Loaded and saved XMl are not the same" );

    //-------------------------------
    // Next: collapse again Subtree [DoorClosed]
    tree = getAbstractTree("MainTree");

    subtree_abs_node = tree.findFirstNode("DoorClosed");
    QVERIFY2(subtree_abs_node, "Can't find node with ID [DoorClosed]");

    {
        auto data_model = subtree_abs_node->graphic_node->nodeDataModel();
        auto subtree_model = dynamic_cast<SubtreeNodeModel*>(data_model);
        QVERIFY2(subtree_model, "Node [DoorClosed] is not SubtreeNodeModel");
        QTest::mouseClick( subtree_model->expandButton(), Qt::LeftButton );
    }

    sleepAndRefresh( 500 );
}

void EditorTest::savedFileSameAsOriginal()
{
    QString file_xml = readFile(":/test_xml_key_reordering_issue.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    QString saved_xml = main_win->saveToXML();

    QFile qFile("crossdoor_EditorTest_savedFileSameAsOriginal.xml");
    if (qFile.open(QIODevice::WriteOnly))
    {
        QTextStream out(&qFile);
        out << saved_xml;
        qFile.close();
    }

    sleepAndRefresh( 500 );
    //-------------------------------
    // Compare AbsBehaviorTree

    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );
    auto tree_A1 = getAbstractTree("BehaviorTree");
    auto tree_A2 = getAbstractTree("RunPlannerSubtree");
    auto tree_A3 = getAbstractTree("ExecutePath");

    main_win->loadFromXML( saved_xml );
    auto tree_B1 = getAbstractTree("BehaviorTree");
    auto tree_B2 = getAbstractTree("RunPlannerSubtree");
    auto tree_B3 = getAbstractTree("ExecutePath");

    bool same_maintree = (tree_A1 == tree_B1);
    if( !same_maintree )
    {
        tree_A1.debugPrint();
        tree_B1.debugPrint();
    }
    QVERIFY2( same_maintree, "AbsBehaviorTree comparison fails" );

    bool same_runplanner = tree_A2 == tree_B2;
    if( !same_runplanner )
    {
        tree_A2.debugPrint();
        tree_B2.debugPrint();
    }
    QVERIFY2( same_runplanner, "AbsBehaviorTree comparison fails" );

    bool same_executepath = tree_A2 == tree_B2;
    if( !same_executepath )
    {
        tree_A3.debugPrint();
        tree_B3.debugPrint();
    }
    QVERIFY2( same_executepath, "AbsBehaviorTree comparison fails" );

    //-------------------------------
    // Compare original and save file contents

    QVERIFY2( file_xml.simplified() == saved_xml.simplified(),
             "Loaded and saved XML are not the same" );
}

void EditorTest::loadFailed()
{
    QString file_xml = readFile(":/crossdoor_with_subtree.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    auto tree_A1 = getAbstractTree("MainTree");
    auto tree_A2 = getAbstractTree("DoorClosed");

    // try to load a bad one

     file_xml = readFile(":/issue_3.xml");

     testMessageBox(400, TEST_LOCATION(), [&]()
     {
         // should fail
         main_win->loadFromXML( file_xml );
     });

     // nothing should change!

     auto tree_B1 = getAbstractTree("MainTree");
     auto tree_B2 = getAbstractTree("DoorClosed");
     QVERIFY2( tree_A1 == tree_B1, "AbsBehaviorTree comparison fails" );
     QVERIFY2( tree_A2 == tree_B2, "AbsBehaviorTree comparison fails" );
}

void EditorTest::undoRedo()
{
    QString file_xml = readFile(":/show_all.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    //------------------------------------------
    AbsBehaviorTree abs_tree_A, abs_tree_B;
    {
        auto container = main_win->currentTabInfo();
        auto view = container->view();
        abs_tree_A = getAbstractTree();

        sleepAndRefresh( 500 );

        auto pippo_node = abs_tree_A.findFirstNode("Pippo");
        auto gui_node = pippo_node->graphic_node;
        QPoint pippo_screen_pos = view->mapFromScene(pippo_node->pos);
        const QPoint pos_offset(100,0);

        QTest::mouseClick( view->viewport(), Qt::LeftButton, Qt::NoModifier, pippo_screen_pos );

        QVERIFY2( gui_node->nodeGraphicsObject().isSelected(), "Pippo is not selected");

        auto old_pos = view->mapFromScene( container->scene()->getNodePosition( *gui_node ) );
        testDragObject(view, pippo_screen_pos, pos_offset);
        QPoint new_pos = view->mapFromScene( container->scene()->getNodePosition( *gui_node ) );

        QCOMPARE( old_pos + pos_offset, new_pos);
        sleepAndRefresh( 500 );
    }
    //---------- test undo ----------
    {
        abs_tree_B = getAbstractTree();
        main_win->onUndoInvoked();
        sleepAndRefresh( 500 );

        QCOMPARE( abs_tree_A, getAbstractTree() );
        sleepAndRefresh( 500 );
    }

    {
        main_win->onUndoInvoked();
        sleepAndRefresh( 500 );
        auto empty_abs_tree = getAbstractTree();
        QCOMPARE( empty_abs_tree.nodesCount(), size_t(1) );

        // nothing should happen
        main_win->onUndoInvoked();
        main_win->onUndoInvoked();
        main_win->onUndoInvoked();
    }

    {
        main_win->onRedoInvoked();
        sleepAndRefresh( 500 );
        QCOMPARE( getAbstractTree(), abs_tree_A);

        main_win->onRedoInvoked();
        sleepAndRefresh( 500 );
        QCOMPARE( getAbstractTree(), abs_tree_B);

        // nothing should happen
        main_win->onRedoInvoked();
        main_win->onRedoInvoked();
        main_win->onRedoInvoked();
    }

    {
        auto container = main_win->currentTabInfo();
        auto view = container->view();

        auto prev_tree = getAbstractTree();
        size_t prev_node_count = prev_tree.nodesCount();

        auto node = prev_tree.findFirstNode( "DoSequenceStar" );

        QPoint pos = view->mapFromScene(node->pos);
        testMouseEvent(view, QEvent::MouseButtonDblClick, pos , Qt::LeftButton);
        sleepAndRefresh( 500 );

        QTest::keyPress( view->viewport(), Qt::Key_Delete, Qt::NoModifier );
        sleepAndRefresh( 500 );

        auto smaller_tree = getAbstractTree();
        QCOMPARE( prev_node_count - 4 , smaller_tree.nodesCount() );

        main_win->onUndoInvoked();
        sleepAndRefresh( 500 );

        auto undo_tree = getAbstractTree();
        size_t undo_node_count = main_win->currentTabInfo()->scene()->nodes().size();
        QCOMPARE( prev_node_count , undo_node_count );

        QCOMPARE( prev_tree, undo_tree);
        main_win->onRedoInvoked();
        auto redo_tree = getAbstractTree();
        sleepAndRefresh( 500 );

        QCOMPARE( smaller_tree, redo_tree);
    }
    sleepAndRefresh( 500 );
}

void EditorTest::renameTabs()
{
    QString file_xml = readFile(":/crossdoor_with_subtree.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    testMessageBox(500, TEST_LOCATION(), [&]()
    {
        // Two tabs with same name would exist
        main_win->onTabRenameRequested( 0 , "DoorClosed" );
    });
    testMessageBox(500, TEST_LOCATION(), [&]()
    {
        // Two tabs with same name would exist
        main_win->onTabRenameRequested( 1 , "MainTree" );
    });

    main_win->onTabRenameRequested( 0 , "MainTree2" );
    main_win->onTabRenameRequested( 1 , "DoorClosed2" );

    QVERIFY( main_win->getTabByName("MainTree") == nullptr);
    QVERIFY( main_win->getTabByName("DoorClosed") == nullptr);

    QVERIFY( main_win->getTabByName("MainTree2") != nullptr);
    QVERIFY( main_win->getTabByName("DoorClosed2") != nullptr);

     sleepAndRefresh( 500 );
}

void EditorTest::testSubtree()
{
    QString file_xml = readFile(":/crossdoor_with_subtree.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    auto main_tree   = getAbstractTree("MainTree");
    auto closed_tree = getAbstractTree("DoorClosed");

    auto subtree_abs_node = main_tree.findFirstNode("DoorClosed");
    auto data_model = subtree_abs_node->graphic_node->nodeDataModel();
    auto subtree_model = dynamic_cast<SubtreeNodeModel*>(data_model);

    QTest::mouseClick( subtree_model->expandButton(), Qt::LeftButton );

    QAbstractButton *button_lock = main_win->findChild<QAbstractButton*>("buttonLock");
    QVERIFY2(button_lock != nullptr, "Can't find the object [buttonLock]");

    button_lock->setChecked(false);

    QTreeWidget* treeWidget = main_win->findChild<QTreeWidget*>("paletteTreeWidget");
    QVERIFY2(treeWidget != nullptr, "Can't find the object [paletteTreeWidget]");

    auto subtree_items = treeWidget->findItems("DoorClosed", Qt::MatchExactly | Qt::MatchRecursive);
    sleepAndRefresh( 500 );

    QCOMPARE(subtree_items.size(), 1);
    sleepAndRefresh( 500 );

    auto sidepanel_editor = main_win->findChild<SidepanelEditor*>("SidepanelEditor");
    sidepanel_editor->onRemoveModel("DoorClosed");
    sleepAndRefresh( 500 );

    auto tree_after_remove = getAbstractTree("MainTree");
    auto fallback_node = tree_after_remove.findFirstNode("root_Fallback");

    QCOMPARE(fallback_node->children_index.size(), size_t(3) );
    QVERIFY2( main_win->getTabByName("DoorClosed") == nullptr, "Tab DoorClosed not deleted");
    sleepAndRefresh( 500 );

    //---------------------------------------
    // create again the subtree

    auto closed_sequence_node = tree_after_remove.findFirstNode("door_closed_sequence");
    auto container = main_win->currentTabInfo();
    // This must fail and create a MessageBox
    testMessageBox(1000, TEST_LOCATION(), [&]()
    {
        container->createSubtree( *closed_sequence_node->graphic_node, "MainTree" );
    });

    container->createSubtree( *closed_sequence_node->graphic_node, "DoorClosed" );
    sleepAndRefresh( 500 );

    auto new_main_tree   = getAbstractTree("MainTree");
    auto new_closed_tree = getAbstractTree("DoorClosed");

    bool is_same = (main_tree == new_main_tree);
    if( !is_same )
    {
        main_tree.debugPrint();
        new_main_tree.debugPrint();
    }
    QVERIFY2( is_same, "AbsBehaviorTree comparison fails" );

    is_same = closed_tree == new_closed_tree;
    if( !is_same )
    {
        closed_tree.debugPrint();
        new_closed_tree.debugPrint();
    }
    QVERIFY2( is_same, "AbsBehaviorTree comparison fails" );
    sleepAndRefresh( 500 );
}

void EditorTest::modifyCustomModel()
{
    QString file_xml = readFile(":/crossdoor_with_subtree.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    QAbstractButton *button_lock = main_win->findChild<QAbstractButton*>("buttonLock");
    button_lock->setChecked(false);

    auto sidepanel_editor = main_win->findChild<SidepanelEditor*>("SidepanelEditor");
    auto treeWidget = sidepanel_editor->findChild<QTreeWidget*>("paletteTreeWidget");

    NodeModel jump_model = { NodeType::ACTION,
                             "JumpOutWindow",
                             { {"UseParachute", PortModel() } }
    };

    sidepanel_editor->onReplaceModel("PassThroughWindow", jump_model);

    auto pass_window_items = treeWidget->findItems("PassThroughWindow",
                                                   Qt::MatchExactly | Qt::MatchRecursive);
    QCOMPARE( pass_window_items.empty(), true);

    auto jump_window_items = treeWidget->findItems( jump_model.registration_ID,
                                                    Qt::MatchExactly | Qt::MatchRecursive);
    QCOMPARE( jump_window_items.size(), 1);

    auto abs_tree = getAbstractTree();

    auto jump_abs_node = abs_tree.findFirstNode( jump_model.registration_ID );
    QVERIFY( jump_abs_node != nullptr);
    sleepAndRefresh( 500 );
    QCOMPARE( jump_abs_node->model, jump_model );

    sleepAndRefresh( 500 );
}

void EditorTest::multipleSubtrees()
{
    QString file_xml = readFile(":/test_subtrees_issue_8.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    auto abs_tree = getAbstractTree("MainTree");

    auto sequence_node = abs_tree.findFirstNode("main_sequence");
    QVERIFY( sequence_node != nullptr );
    QCOMPARE( sequence_node->children_index.size(), size_t(2) );

    int index_1 = sequence_node->children_index[0];
    int index_2 = sequence_node->children_index[1];

    auto first_child  = abs_tree.node(index_1);
    auto second_child = abs_tree.node(index_2);

    QCOMPARE( first_child->instance_name,  tr("MoveToPredefinedPoint") );
    QCOMPARE( second_child->instance_name, tr("SubtreeOne") );

    sleepAndRefresh( 500 );
}

void EditorTest::editText()
{
    QString file_xml = readFile("://show_all.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    auto abs_tree = getAbstractTree();

    std::list<QLineEdit*> line_editable;

    for(const auto& node: abs_tree.nodes())
    {
        auto lines = node.graphic_node->nodeDataModel()->embeddedWidget()->findChildren<QLineEdit*>();
        for(const auto& line: lines)
        {
            if( line->isReadOnly() == false && line->isHidden() == false)
            {
                line_editable.push_back( line );
            }
        }
    }

    auto container = main_win->currentTabInfo();
    auto view = container->view();

    for(const auto& line: line_editable)
    {
        QTest::mouseDClick( line, Qt::LeftButton );
        line->selectAll();
        sleepAndRefresh( 50 );

        QTest::keyClick(line, Qt::Key_Delete, Qt::NoModifier );
        sleepAndRefresh( 50 );
        QCOMPARE( line->text(), QString() );

        QTest::mouseClick( line, Qt::LeftButton );
        sleepAndRefresh( 50 );
        QTest::keyClicks(view->viewport(), "was_here");
        QCOMPARE( line->text(), tr("was_here") );
    }

    sleepAndRefresh( 500 );
}

void EditorTest::loadModelLess()
{
    QString file_xml = readFile("://simple_without_model.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    auto models = main_win->registeredModels();

    QVERIFY( models.find("moverobot") != models.end() );

    const auto& moverobot_model = models.at("moverobot");

//    QCOMPARE( moverobot_model.params.size(),  size_t(1) );
//    QCOMPARE( moverobot_model.params.front().label, tr("location") );
//    QCOMPARE( moverobot_model.params.front().value, tr("1") );

}

void EditorTest::longNames()
{
    QString file_xml = readFile(":/issue_24.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    auto abs_tree = getAbstractTree();
    QCOMPARE( abs_tree.nodesCount(), size_t(4) );
    auto sequence = abs_tree.node(1);
    QCOMPARE( sequence->model.registration_ID, QString("Sequence"));

    // second child on the right side.
    int short_index = sequence->children_index[1];
    auto short_node = abs_tree.node(short_index);
    QCOMPARE( short_node->model.registration_ID, QString("short") );
}

void EditorTest::clearModels()
{
    QString file_xml = readFile(":/crossdoor_with_subtree.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    file_xml = readFile(":/show_all.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    auto container = main_win->currentTabInfo();
   // auto view = container->view();

    auto abs_tree = getAbstractTree();
    auto node = abs_tree.findFirstNode( "DoSequenceStar" );

    QTimer::singleShot(300, [&]()
    {
        // No message box expected
        QWidget* modal_widget = QApplication::activeModalWidget();

        if (dynamic_cast<QMessageBox*>(modal_widget))
        {
            QKeyEvent* event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
            QCoreApplication::postEvent(modal_widget, event);
            QFAIL("no QMessageBox");
        }
    });

    container->createSubtree( *node->graphic_node, "DoorClosed" );
}

void EditorTest::undoWithSubtreeExpanded()
{
    QString file_xml = readFile(":/crossdoor_with_subtree.xml");
    main_win->on_actionClear_triggered();
    main_win->loadFromXML( file_xml );

    auto abs_tree = getAbstractTree("MainTree");
    auto subtree_node = abs_tree.findFirstNode("DoorClosed")->graphic_node;
    auto window_node  = abs_tree.findFirstNode("PassThroughWindow")->graphic_node;

    auto subtree_model = dynamic_cast<SubtreeNodeModel*>( subtree_node->nodeDataModel() );
    QTest::mouseClick( subtree_model->expandButton(), Qt::LeftButton );

    sleepAndRefresh( 500 );

    auto scene = main_win->getTabByName("MainTree")->scene();
    int node_count_A = scene->nodes().size();
    scene->removeNode(*window_node);

    sleepAndRefresh( 500 );

    abs_tree = getAbstractTree("MainTree");
    int node_count_B = scene->nodes().size();
    QCOMPARE( node_count_A -1 , node_count_B );

    main_win->onUndoInvoked();
    scene = main_win->getTabByName("MainTree")->scene();
    int node_count_C = scene->nodes().size();
     QCOMPARE( node_count_A , node_count_C );

     sleepAndRefresh( 500 );
}

QTEST_MAIN(EditorTest)

#include "editor_test.moc"
