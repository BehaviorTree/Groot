#ifndef GROOT_TEST_BASE_H
#define GROOT_TEST_BASE_H

#include <QtTest>
#include <QGuiApplication>
#include <QGraphicsView>
#include <QMouseEvent>
#include <QApplication>

#include "bt_editor/mainwindow.h"
#include "bt_editor/utils.h"
#include "bt_editor/models/SubtreeNodeModel.hpp"


class GrootTestBase : public QObject
{
    Q_OBJECT

protected:
    QString readFile(const char* name);

    void sleepAndRefresh(int ms);

    void testMouseEvent(QGraphicsView* view, QEvent::Type type, QPoint pos,
                        Qt::MouseButton button,
                        Qt::KeyboardModifier modifier = Qt::NoModifier);

    void testDragObject(QGraphicsView* view, const QPoint& screen_pos, const QPoint& pos_offset);

    AbsBehaviorTree getAbstractTree(const QString& name = QString());

    MainWindow *main_win;
};

#endif // GROOT_TEST_BASE_H
