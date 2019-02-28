#include "groot_test_base.h"
#include "bt_editor/sidepanel_replay.h"
#include <QAction>

class ReplyTest : public GrootTestBase
{
    Q_OBJECT

public:
    ReplyTest() {}
    ~ReplyTest() {}

private slots:
    void initTestCase();
    void cleanupTestCase();
    void basicLoad();
};


void ReplyTest::initTestCase()
{
    main_win = new MainWindow(GraphicMode::REPLAY, nullptr);
    main_win->resize(1200, 800);
    main_win->show();
}

void ReplyTest::cleanupTestCase()
{
    QApplication::processEvents();
    sleepAndRefresh( 1000 );
    main_win->on_actionClear_triggered();
    main_win->close();
}

void ReplyTest::basicLoad()
{
    auto sidepanel_replay = main_win->findChild<SidepanelReplay*>("SidepanelReplay");
    QVERIFY2( sidepanel_replay, "Can't get pointer to SidepanelReplay" );

    QByteArray log = readFile("://crossdoor_trace.fbl");
    sidepanel_replay->loadLog( log );

    QCOMPARE( sidepanel_replay->transitionsCount(), size_t(27) );
}

QTEST_MAIN(ReplyTest)

#include "replay_test.moc"
