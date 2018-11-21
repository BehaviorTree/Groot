#include "groot_test_base.h"
#include <QTimer>
#include <QKeyEvent>

QByteArray GrootTestBase::readFile(const char *name)
{
    QString fileName(name);
    QFile file(fileName);
    bool ret = file.open(QIODevice::ReadOnly);
    QByteArray data = file.readAll();
    file.close();
    if( !ret ){
        QTest::qFail("can't load file", __FILE__, __LINE__);
    }
    return data;
}

void GrootTestBase::sleepAndRefresh(int ms)
{
    QApplication::processEvents();
    QTest::qSleep ( ms );
    QApplication::processEvents();
}

void GrootTestBase::testMouseEvent(QGraphicsView *view, QEvent::Type type, QPoint pos, Qt::MouseButton button,
                                   Qt::KeyboardModifier modifier)
{
    auto event = new QMouseEvent(type, pos, view->viewport()->mapToGlobal(pos),
                                 button, button, modifier);

    QApplication::postEvent(view->viewport(), event);
    QApplication::processEvents();
}

void GrootTestBase::testDragObject(QGraphicsView *view, const QPoint &screen_pos, const QPoint &pos_offset)
{
    testMouseEvent(view, QEvent::MouseButtonPress,    screen_pos , Qt::LeftButton);
    testMouseEvent(view, QEvent::MouseMove,           screen_pos + pos_offset, Qt::LeftButton);
    testMouseEvent(view, QEvent::MouseButtonRelease,  screen_pos + pos_offset, Qt::LeftButton);
}

AbsBehaviorTree GrootTestBase::getAbstractTree(const QString &name)
{
    if(name.isEmpty() )
        return BuildTreeFromScene( main_win->currentTabInfo()->scene() );
    else
        return BuildTreeFromScene( main_win->getTabByName(name)->scene() );
}

void GrootTestBase::testMessageBox(int deplay_ms, TestLocation location,
                                   std::function<void()> callable_action,
                                   QMessageBox::StandardButton button_to_press)
{
    QMutex semaphore;
    semaphore.lock();

    QTimer::singleShot(deplay_ms, [&]()
    {
        QWidget* widget = QApplication::activeModalWidget();
        if (!widget){
            QTest::qFail("no modal widget", location.file, location.line);
        }
        auto message_box = dynamic_cast<QMessageBox*>(widget);
        if (!message_box){
            QTest::qFail("no QMessageBox", location.file, location.line);
        }
        if( button_to_press == QMessageBox::Default || button_to_press == QMessageBox::NoButton)
        {
            QKeyEvent* event = new QKeyEvent(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
            QCoreApplication::postEvent(widget, event);
        }
        else{
            emit message_box->button(button_to_press)->clicked();
        }
        semaphore.unlock();
    });
    callable_action();
    semaphore.tryLock( deplay_ms * 2 );
    semaphore.unlock();
}
