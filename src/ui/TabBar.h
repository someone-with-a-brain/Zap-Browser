#pragma once

/**
 * TabBar — Custom tab widget for ZAP.
 *
 * Features:
 *  - Favicon + title per tab
 *  - Close button on each tab
 *  - "New Tab" (+) button
 *  - Tab sleeping indicator (moon icon on sleeping tabs)
 *  - Context menu: duplicate, pin, close to right
 */

#include <QWidget>
#include <QTabBar>
#include <QPushButton>
#include <QHBoxLayout>
#include <QIcon>
#include <QMap>

class TabBar : public QWidget {
    Q_OBJECT

public:
    explicit TabBar(QWidget* parent = nullptr);

    int  AddTab(const QString& title, const QIcon& icon = QIcon());
    void RemoveTab(int index);
    void SetTabTitle(int index, const QString& title);
    void SetTabIcon(int index, const QIcon& icon);
    void SetTabSleeping(int index, bool sleeping);
    int  CurrentIndex() const;
    void SetCurrentIndex(int index);
    int  Count() const;

signals:
    void tabChanged(int index);
    void tabCloseRequested(int index);
    void newTabRequested();

private slots:
    void onCurrentChanged(int index);
    void onTabCloseRequested(int index);
    void onNewTabClicked();
    void onContextMenuRequested(const QPoint& pos);

private:
    QTabBar*    tabBar_      = nullptr;
    QPushButton* newTabBtn_  = nullptr;

    QMap<int, bool> sleepingTabs_;
};
