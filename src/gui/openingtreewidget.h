/****************************************************************************
*   Copyright (C) 2012 by Jens Nissen jens-chessx@gmx.net                   *
****************************************************************************/

#ifndef OPENINGTREEWIDGET_H
#define OPENINGTREEWIDGET_H

#include "board.h"
#include "filter.h"
#include <QAbstractItemModel>
#include <QPointer>
#include <QUndoCommand>
#include <QWidget>

namespace Ui
{
class OpeningTreeWidget;
}

class BoardView;
class OpeningTree;
class QUndoStack;

class OpeningTreeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OpeningTreeWidget(QWidget *parent = nullptr);
    ~OpeningTreeWidget();
    OpeningTree* m_openingTree;

    QString move(QModelIndex index) const;
    BoardX board() const;
    void updateFilter(FilterX& f, const BoardX& b, bool bEnd);
    void doSetBoard(FilterX& f, const BoardX& b, bool bEnd);
    int getFilterIndex(QString& name) const;
    bool shouldAddMove() const;

public slots:
    void cancel();
    /** Store current configuration. */
    void saveConfig();
    /** Restore current configuration. */
    void slotReconfigure();
    /** Update the list of databases */
    void updateFilterIndex(QStringList);
    /** Restore Book settings */
    void restoreBook();
    void terminate();
protected slots:
    /** Indicate end of tree update */
    void slotTreeUpdate();
    /** Show progress bar for open file. */
    void slotOperationProgress(int value);
    /** Indicate start of tree update */
    void slotTreeUpdateStarted();
    /** The source for the tree has changed */
    void slotSourceChanged(int);
    /** Updating the filter is requested by clicking the filter checkbox */
    void slotFilterClicked(bool checked);

signals:
    void signalTreeUpdated(bool);
    void signalSourceChanged();
    void requestGameFilterUpdate(int,int);

protected:
    bool filterGames() const;

private:
    Ui::OpeningTreeWidget *ui;
    BoardView* m_openingBoardView;
    QStringList m_filePaths;
    QUndoStack* m_UndoStack;
};

class BoardUndoCommand : public QUndoCommand
{
public:
    BoardUndoCommand(QObject* parent, FilterX* f, const BoardX& b, bool bEnd, QString action) :
        QUndoCommand(action),
        m_openingTree((OpeningTreeWidget*)parent),
        m_filter(f), m_board(b), m_bEnd(bEnd)
        {
        }

    QPointer<OpeningTreeWidget> m_openingTree;
    QPointer<FilterX> m_filter;
    BoardX m_board;
    bool m_bEnd;

    virtual void undo() { if (m_filter && m_openingTree) m_openingTree->doSetBoard(*m_filter, m_board, m_bEnd); }
    virtual void redo() { }
};

#endif // OPENINGTREEWIDGET_H
