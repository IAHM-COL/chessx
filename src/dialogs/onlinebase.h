#ifndef ONLINEBASE_H
#define ONLINEBASE_H

#include <QDialog>
#include <QDate>

namespace Ui {
class OnlineBase;
}

class OnlineBase : public QDialog
{
    Q_OBJECT

public:
    explicit OnlineBase(QWidget *parent = nullptr);
    ~OnlineBase();

    QDate getStartDate() const;
    void setStartDate(const QDate &value);

    QString getHandle() const;
    void setHandle(const QString &value);

private:
    Ui::OnlineBase *ui;
};

#endif // ONLINEBASE_H