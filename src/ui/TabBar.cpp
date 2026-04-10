#include "TabBar.h"

#include <QHBoxLayout>
#include <QMenu>
#include <QAction>
#include <QContextMenuEvent>
#include <QDebug>

TabBar::TabBar(QWidget* parent)
    : QWidget(parent)
{
    setObjectName("tab-bar-widget");

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    tabBar_ = new QTabBar(this);
    tabBar_->setObjectName("tab-bar");
    tabBar_->setTabsClosable(true);
    tabBar_->setMovable(true);
    tabBar_->setExpanding(false);
    tabBar_->setUsesScrollButtons(true);

    connect(tabBar_, &QTabBar::currentChanged,
            this, &TabBar::onCurrentChanged);
    connect(tabBar_, &QTabBar::tabCloseRequested,
            this, &TabBar::onTabCloseRequested);
    tabBar_->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(tabBar_, &QTabBar::customContextMenuRequested,
            this, &TabBar::onContextMenuRequested);

    newTabBtn_ = new QPushButton("+", this);
    newTabBtn_->setObjectName("new-tab-btn");
    newTabBtn_->setFixedSize(28, 28);
    newTabBtn_->setToolTip("New Tab (Ctrl+T)");
    connect(newTabBtn_, &QPushButton::clicked,
            this, &TabBar::onNewTabClicked);

    layout->addWidget(tabBar_, 1);
    layout->addWidget(newTabBtn_);
    setLayout(layout);
}

int TabBar::AddTab(const QString& title, const QIcon& icon)
{
    int idx = icon.isNull()
        ? tabBar_->addTab(title)
        : tabBar_->addTab(icon, title);
    tabBar_->setCurrentIndex(idx);
    return idx;
}

void TabBar::RemoveTab(int index)
{
    sleepingTabs_.remove(index);
    tabBar_->removeTab(index);
}

void TabBar::SetTabTitle(int index, const QString& title)
{
    QString truncated = title.length() > 28
        ? title.left(25) + "..."
        : title;
    tabBar_->setTabText(index, truncated);
    tabBar_->setTabToolTip(index, title);
}

void TabBar::SetTabIcon(int index, const QIcon& icon)
{
    tabBar_->setTabIcon(index, icon);
}

void TabBar::SetTabSleeping(int index, bool sleeping)
{
    sleepingTabs_[index] = sleeping;
    // Append moon emoji to sleeping tab to signal it
    QString text = tabBar_->tabText(index);
    if (sleeping && !text.startsWith("💤 "))
        tabBar_->setTabText(index, "💤 " + text);
    else if (!sleeping && text.startsWith("💤 "))
        tabBar_->setTabText(index, text.mid(4));
}

int TabBar::CurrentIndex() const
{
    return tabBar_->currentIndex();
}

void TabBar::SetCurrentIndex(int index)
{
    tabBar_->setCurrentIndex(index);
}

int TabBar::Count() const
{
    return tabBar_->count();
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void TabBar::onCurrentChanged(int index)
{
    emit tabChanged(index);
}

void TabBar::onTabCloseRequested(int index)
{
    emit tabCloseRequested(index);
}

void TabBar::onNewTabClicked()
{
    emit newTabRequested();
}

void TabBar::onContextMenuRequested(const QPoint& pos)
{
    int idx = tabBar_->tabAt(pos);
    if (idx < 0) return;

    QMenu menu(this);
    QAction* closeThisAct   = menu.addAction("Close Tab");
    QAction* closeOthersAct = menu.addAction("Close Other Tabs");
    menu.addSeparator();
    QAction* duplicateAct   = menu.addAction("Duplicate Tab");
    QAction* muteAct        = menu.addAction(
        sleepingTabs_.value(idx) ? "Wake Tab" : "Sleep Tab");

    QAction* chosen = menu.exec(tabBar_->mapToGlobal(pos));
    if (!chosen) return;

    if (chosen == closeThisAct)     emit tabCloseRequested(idx);
    if (chosen == closeOthersAct) {
        for (int i = tabBar_->count() - 1; i >= 0; --i) {
            if (i != idx) emit tabCloseRequested(i);
        }
    }
    if (chosen == duplicateAct)    emit newTabRequested();
    if (chosen == muteAct)         SetTabSleeping(idx, !sleepingTabs_.value(idx));
}
