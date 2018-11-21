#ifndef GROOT_TEST_BASE_H
#define GROOT_TEST_BASE_H

#include <QtTest>
#include <QGuiApplication>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QApplication>
#include <QMessageBox>
#include "bt_editor/mainwindow.h"
#include "bt_editor/utils.h"
#include "bt_editor/models/SubtreeNodeModel.hpp"

#define TEST_LOCATION() {  __FILE__, __LINE__ }

class GrootTestBase : public QObject
{
    Q_OBJECT

public:
    struct TestLocation{
        const char* file;
        int line;
    };

protected:
    QByteArray readFile(const char* name);

    void sleepAndRefresh(int ms);

    void testMouseEvent(QGraphicsView* view, QEvent::Type type, QPoint pos,
                        Qt::MouseButton button,
                        Qt::KeyboardModifier modifier = Qt::NoModifier);

    void testDragObject(QGraphicsView* view, const QPoint& screen_pos, const QPoint& pos_offset);

    AbsBehaviorTree getAbstractTree(const QString& name = QString());

    void testMessageBox(int deplay_ms, TestLocation location,
                        std::function<void()> callable_action,
                        QMessageBox::StandardButton button_to_press = QMessageBox::Default);

    MainWindow *main_win;
};

#endif // GROOT_TEST_BASE_H
