/****************************************************************************
*   Copyright (C) 2013 by Jens Nissen jens-chessx@gmx.net                   *
****************************************************************************/

#include "aboutdlg.h"
#include "databaseinfo.h"
#include "settings.h"
#include "shellhelper.h"
#include "version.h"

#include "ui_aboutdlg.h"
#include <QSslSocket>

#if defined(_MSC_VER) && defined(_DEBUG)
#define DEBUG_NEW new( _NORMAL_BLOCK, __FILE__, __LINE__ )
#define new DEBUG_NEW
#endif // _MSC_VER

static QString pathify(QString s)
{
    return QString("<a href='%1'>%2</a>").arg(s, s);
}

AboutDlg::AboutDlg(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDlg)
{
    ui->setupUi(this);
    restoreLayout();

    QString refPath = AppSettings->dataPath();
    ui->labelDataPath->setText(pathify(refPath));

    refPath = AppSettings->commonDataPath();
    ui->labelDocPath->setText(pathify(refPath));

    refPath = AppSettings->getTempPath();
    ui->labelTempPath->setText(pathify(refPath));
    ui->labelFicsPath->setText(DatabaseInfo::ficsPath());

    ui->labelCopyRightDate->setText(COPYRIGHT_DATE);

    QString version = QString(STR_REVISION).replace(',', '.');
#ifdef Q_OS_UNIX
#ifndef Q_OS_MAC
    version += " UNIX";
#endif
#endif
#ifdef Q_OS_MACOS
    version += " MAC";
#endif
#ifdef Q_OS_WIN
    version += " WIN";
#endif
#ifdef QT_POINTER_SIZE
    version.append(QString::number(8*QT_POINTER_SIZE));
#endif
#ifdef Q_CC_GNU
    version += " GNU";
#endif
#if _MSC_VER && !__INTEL_COMPILER
    version.append(QString(" MSVC%1").arg(_MSC_VER/100-6));
#endif
#ifdef QT_VERSION_STR
    version += " Qt";
    version += QT_VERSION_STR;
#endif

    ui->labelVersion->setText(version);

    QString settingsPath = AppSettings->fileName();
    ui->labelSettingsPath->setText(pathify(settingsPath));

    ui->labelFicsPath->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    ui->labelTempPath->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard);
    ui->labelDocPath->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard);
    ui->labelDataPath->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard);
    ui->labelSettingsPath->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard);
    ui->labelHomepage->setTextInteractionFlags(Qt::TextBrowserInteraction);
    ui->labelMailingList->setTextInteractionFlags(Qt::TextBrowserInteraction);

    ui->lbSslQt->setText(QSslSocket::sslLibraryBuildVersionString());
    ui->lbSslSupport->setText(QSslSocket::supportsSsl()?tr("Yes"):tr("No"));
    ui->lbSslLibrary->setText(QSslSocket::sslLibraryVersionString());
}

void AboutDlg::restoreLayout()
{
    AppSettings->layout(this);
}

AboutDlg::~AboutDlg()
{
    delete ui;
}

void AboutDlg::accept()
{
    AppSettings->setLayout(this);
    QDialog::accept();
}

void AboutDlg::reject()
{
    AppSettings->setLayout(this);
    QDialog::reject();
}

void AboutDlg::on_labelTempPath_linkActivated(const QString &link)
{
    ShellHelper::showInFinder(link);
}

void AboutDlg::on_labelDataPath_linkActivated(const QString &link)
{
    ShellHelper::showInFinder(link);
}

void AboutDlg::on_labelSettingsPath_linkActivated(const QString &link)
{
    ShellHelper::showInFinder(link);
}

void AboutDlg::on_labelDocPath_linkActivated(const QString &link)
{
    ShellHelper::showInFinder(link);
}
