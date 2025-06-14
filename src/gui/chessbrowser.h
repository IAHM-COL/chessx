/***************************************************************************
								  chessbrowser.h  -  Tweaked QTextBrowser
									  -------------------
	 begin                : Thu 31 Aug 2006
	 copyright            : (C) 2006 Michal Rudolf <mrudolf@kdewebdev.org>
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/


#ifndef CHESSBROWSER_H_INCLUDED
#define CHESSBROWSER_H_INCLUDED

#include <QGestureEvent>
#include <QtGui>
#include <QTextBrowser>
#include "editaction.h"
#include "gameid.h"

class DatabaseInfo;

/** @ingroup GUI
	The ChessBrowser class is a slightly modified QTextBrowser
	that handles internal pseudo-links. */
class ChessBrowser : public QTextBrowser
{
    Q_OBJECT
public:
    /** Constructs new instance with parent @p parent. */
    ChessBrowser(QWidget* p);
    QMap<QAction*, EditAction> m_actions;

    QStringList getAnchors(QList<MoveId> list);
public slots:
    /** Scroll to show given mode. */
    void showMove(int id);

    /** Invoke action */
    void slotAction(QAction* action);
    /** Show menu */
    void slotContextMenu(const QPoint& pos);
signals:
    void actionRequested(const EditAction& action);
    void queryActiveGame(const GameX** game);
    void signalMergeGame(GameId gameIndex, QString source);
    void swipeRight();
    void swipeLeft();

protected:
    virtual bool selectAnchor(const QString& href);
    virtual void doSetSource(const QUrl &name, QTextDocument::ResourceType type = QTextDocument::UnknownResource);
    virtual void setSource(const QUrl& url);
    void setupMenu();
    QAction* createAction(const QString& name, EditAction::Type type);
    QAction* createNagAction(const Nag& nag);

protected: // Drag+Drop
    void dragEnterEvent(QDragEnterEvent *event);
    void dragMoveEvent(QDragMoveEvent *event);
    void dragLeaveEvent(QDragLeaveEvent *event);
    void dropEvent(QDropEvent *event);
    void mergeGame(GameId gameIndex);

    QStringList getAnchors(const QStringList &hrefs);
    void handleSwipe(QSwipeGesture *gesture);
    bool gestureEvent(QGestureEvent *event);
    bool event(QEvent *event);
private:

    QAction* m_copyHtml;
    QAction* m_copyText;
    QAction* m_uncomment;
    QAction* m_remove;
    QAction* m_startComment;
    QAction* m_gameComment;
    QAction* m_gameComment2;
    QAction* m_addComment;
    QAction* m_removeVariation;
    QAction* m_promoteVariation;
    QAction* m_VariationUp;
    QAction* m_VariationDown;
    QAction* m_removePrevious;
    QAction* m_removeNext;
    QAction* m_addNullMove;
    QAction* m_addNullMove2;
    QAction* m_removeNags;
    QAction* m_enumerateVariations1;
    QAction* m_enumerateVariations2;
    QAction* m_copyTextSelection;
    QMenu* m_gameMenu;
    QMenu* m_browserMenu;
    QMenu* m_mainMenu;
    int m_currentMove;
};

#endif

