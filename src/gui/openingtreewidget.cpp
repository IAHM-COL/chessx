/****************************************************************************
*   Copyright (C) 2012 by Jens Nissen jens-chessx@gmx.net                   *
****************************************************************************/

#include "openingtreewidget.h"
#include "ui_openingtreewidget.h"

#include <QMetaType>
#include <QModelIndex>
#include <QToolButton>
#include <QUndoGroup>
#include <QUndoStack>

#include "boardview.h"
#include "databaseinfo.h"
#include "htmlitemdelegate.h"
#include "openingtree.h"
#include "settings.h"

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

OpeningTreeWidget::OpeningTreeWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::OpeningTreeWidget)
{
    ui->setupUi(this);

    qRegisterMetaType<QList<MoveData> >("QList<MoveData>");
    qRegisterMetaType<QList<Move> >("QList<Move>");

    m_openingTree = new OpeningTree(ui->OpeningTreeView);

    QUndoGroup* undoGroup = new QUndoGroup(this);
    m_UndoStack = new QUndoStack(undoGroup);
    m_UndoStack->setUndoLimit(100);
    undoGroup->addStack(m_UndoStack);
    undoGroup->setActiveStack(m_UndoStack);

    connect(ui->btUndo, SIGNAL(clicked()), undoGroup, SLOT(undo()));
    connect(undoGroup, SIGNAL(canUndoChanged(bool)), ui->btUndo, SLOT(setEnabled(bool)));
    ui->btUndo->setEnabled(false);

    ui->OpeningTreeView->setObjectName("OpeningTree");
    ui->OpeningTreeView->setSortingEnabled(true);
    ui->OpeningTreeView->setModel(m_openingTree);
    ui->OpeningTreeView->sortByColumn(1, Qt::DescendingOrder);

    HTMLItemDelegate* htmlItemDelegate = new HTMLItemDelegate(this);
    ui->OpeningTreeView->setItemDelegate(htmlItemDelegate);

    connect(ui->OpeningTreeView, SIGNAL(clicked(QModelIndex)), parent, SLOT(slotSearchTreeMove(QModelIndex)));
    connect(m_openingTree, SIGNAL(progress(int)), this, SLOT(slotOperationProgress(int)));
    connect(m_openingTree, SIGNAL(openingTreeUpdated()), this, SLOT(slotTreeUpdate()));
    connect(m_openingTree, SIGNAL(openingTreeUpdateStarted()), this, SLOT(slotTreeUpdateStarted()));
    connect(m_openingTree, SIGNAL(requestGameFilterUpdate(int,int)), SIGNAL(requestGameFilterUpdate(int,int)));
    connect(ui->sourceSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(slotSourceChanged(int)));
    connect(ui->filterGames, SIGNAL(clicked(bool)), this, SLOT(slotFilterClicked(bool)));
    m_openingBoardView = new BoardView(this, BoardView::IgnoreSideToMove | BoardView::SuppressGuessMove);
    m_openingBoardView->setObjectName("OpeningBoardView");
    m_openingBoardView->setMinimumSize(200, 200);
    ui->OpeningBoardWidget->addWidget(m_openingBoardView, 1);
    m_openingBoardView->configure();
    m_openingBoardView->setEnabled(false);
}

OpeningTreeWidget::~OpeningTreeWidget()
{
    delete ui;
}

void OpeningTreeWidget::cancel()
{
    m_openingTree->cancel();
}

void OpeningTreeWidget::terminate()
{
    m_openingTree->terminate();
}

QString OpeningTreeWidget::move(QModelIndex index) const
{
    return m_openingTree->move(index);
}

BoardX OpeningTreeWidget::board() const
{
    return m_openingTree->board();
}

void OpeningTreeWidget::updateFilter(FilterX& f, const BoardX& b, bool bEnd)
{
    if (ui->btPin->isChecked()) return;

    if (m_openingTree->board() != b)
    {
        m_UndoStack->push(new BoardUndoCommand(this,&f,m_openingTree->board(),m_openingTree->bEnd(),""));
    }
    doSetBoard(f,b,bEnd);
}

bool OpeningTreeWidget::filterGames() const
{
    return ui->filterGames->isEnabled() && ui->filterGames->isChecked();
}

void OpeningTreeWidget::doSetBoard(FilterX& f, const BoardX& b, bool bEnd)
{
    m_openingBoardView->setBoard(b);
    m_openingTree->updateFilter(f, b, filterGames(), ui->sourceSelector->currentIndex()==1, bEnd);
}

void OpeningTreeWidget::saveConfig()
{
    AppSettings->setLayout(this);
    ui->OpeningTreeView->saveConfig();
    AppSettings->beginGroup(objectName());
    AppSettings->setValue("LastBook", ui->sourceSelector->currentText());
    AppSettings->endGroup();
}

void OpeningTreeWidget::slotReconfigure()
{
    m_openingBoardView->configure();
    AppSettings->layout(this);
    ui->OpeningTreeView->slotReconfigure();
}

void OpeningTreeWidget::slotOperationProgress(int value)
{
    ui->progress->setValue(value);
}

void OpeningTreeWidget::slotTreeUpdate()
{
    ui->progress->setValue(100);
    if (filterGames())
    {
        bool dbIsFilterSource = (ui->sourceSelector->currentIndex()<=1);
        emit signalTreeUpdated(dbIsFilterSource);
    }
}

void OpeningTreeWidget::slotTreeUpdateStarted()
{
    ui->progress->setValue(0);
}

int OpeningTreeWidget::getFilterIndex(QString& name) const
{
    int index = ui->sourceSelector->currentIndex();
    if (index>1)
        name = m_filePaths[ui->sourceSelector->currentIndex()-2];
    else
        name = ui->sourceSelector->currentText();
    return index;
}

void OpeningTreeWidget::updateFilterIndex(QStringList files)
{
    m_filePaths.clear();
    m_filePaths = files;
    disconnect(ui->sourceSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(slotSourceChanged(int)));
    QString current = ui->sourceSelector->currentText();
    ui->sourceSelector->clear();
    QStringList baseNames;
    foreach(QString filename, files)
    {
        QFileInfo fi(filename);
        QString baseName = fi.baseName();
        if (DatabaseInfo::IsBook(filename))
        {
            baseName += tr(" (Book)");
        }
        baseNames.append(baseName);
    }

    QStringList allFiles;
    allFiles << tr("Database") << tr("Filter") << baseNames;
    ui->sourceSelector->insertItems(0, allFiles);
    if (allFiles.contains(current))
    {
        int index = ui->sourceSelector->findText(current);
        ui->sourceSelector->setCurrentIndex(index);
    }
    else
    {
        ui->sourceSelector->setCurrentIndex(0);
    }
    connect(ui->sourceSelector, SIGNAL(currentIndexChanged(int)), this, SLOT(slotSourceChanged(int)));
}

void OpeningTreeWidget::restoreBook()
{
    AppSettings->beginGroup(objectName());
    QString lastBook = AppSettings->value("LastBook", "").toString();
    AppSettings->endGroup();
    int index = ui->sourceSelector->findText(lastBook);
    if (index >= 0)
    {
        ui->sourceSelector->setCurrentIndex(index);
    }
}

bool OpeningTreeWidget::shouldAddMove() const
{
    return ui->makeMove->isChecked();
}

void OpeningTreeWidget::slotFilterClicked(bool checked)
{
    if (checked)
    {
        emit signalSourceChanged();
    }
    else
    {
        m_openingTree->cancel();
    }
}

void OpeningTreeWidget::slotSourceChanged(int index)
{
    m_UndoStack->clear();
    m_openingTree->cancel();
    if (index >= 0)
    {
        ui->filterGames->setEnabled(index<=1);
    }
    emit signalSourceChanged();
}
