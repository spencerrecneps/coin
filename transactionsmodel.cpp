#include <QtSql>
#include <QLocale>
#include "transactionsmodel.h"

#define col_pk_uid 0
#define col_id_account 1
#define col_relate_account 2
#define col_date 3
#define col_comment 4
#define col_amount 5
#define col_total 6
#define col_reconciled 7

TransactionsModel::TransactionsModel(QObject *parent) :
    QSqlQueryModel(parent)
{
}

Qt::ItemFlags TransactionsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QSqlQueryModel::flags(index);
    if (index.column() == col_date  //identify editable columns
            || index.column() == col_comment
            || index.column() == col_amount)
    {
        flags |= Qt::ItemIsEditable;  //set columns as editable
    }
    return flags;
}

bool TransactionsModel::setData(const QModelIndex &index, const QVariant &value, int /*role*/)
{
    if (       index.column() == col_pk_uid
            || index.column() == col_id_account
            || index.column() == col_relate_account
            || index.column() == col_total
            || index.column() == col_reconciled)
    {
        return false;
    }
    QModelIndex primaryKeyIndex = QSqlQueryModel::index(index.row(),0);
    int pk_uid = data(primaryKeyIndex, Qt::EditRole).toInt();

    clear();

    bool success;
    if (index.column() == col_date) {
        success = setDate(pk_uid,value.toString());
    }
    else if (index.column() == col_comment) {
        success = setComment(pk_uid,value.toString());
    }
    else if (index.column() == col_amount) {
        double amt;
        amt = value.toFloat();
        success = setAmount(pk_uid,amt);
    }
    else {
        success = false;
    }
    refresh();
    return success;
}

void TransactionsModel::refresh()
{
    setQuery("SELECT pk_uid, id_account, relate_account, date_trans, comment, amount, total, reconciled FROM trans_total ORDER BY date_trans, pk_uid");
    setHeaderData(col_pk_uid,Qt::Horizontal,QObject::tr("pk_uid"));
    setHeaderData(col_id_account,Qt::Horizontal,QObject::tr("id_account"));
    setHeaderData(col_relate_account,Qt::Horizontal,QObject::tr("relate_account"));
    setHeaderData(col_date,Qt::Horizontal,QObject::tr("Date"));
    setHeaderData(col_comment,Qt::Horizontal,QObject::tr("Comment"));
    setHeaderData(col_amount,Qt::Horizontal,QObject::tr("Amount"));
    setHeaderData(col_total,Qt::Horizontal,QObject::tr("Total"));
    setHeaderData(col_reconciled,Qt::Horizontal,QObject::tr("Reconciled"));
}

bool TransactionsModel::setDate(int pk_uid, const QString &transactionDate)
{
    QSqlQuery q;

    //first update the selected transaction
    q.prepare("UPDATE trans SET date_trans = ? WHERE pk_uid = ?");
    q.addBindValue(transactionDate);
    q.addBindValue(pk_uid);
    if (!q.exec()) return false;

    //next update the related transaction (if exists)
    q.clear();
    q.prepare("UPDATE trans SET date_trans = ? WHERE id_relate = ?");
    q.addBindValue(transactionDate);
    q.addBindValue(pk_uid);
    if (!q.exec()) return false;

    return true;
}

bool TransactionsModel::setComment(int pk_uid, const QString &transactionComment)
{
    QSqlQuery q;

    //first update the selected transaction
    q.prepare("UPDATE trans SET comment = ? WHERE pk_uid = ?");
    q.addBindValue(transactionComment);
    q.addBindValue(pk_uid);
    if (!q.exec()) return false;

    //next update the related transaction (if exists)
    q.clear();
    q.prepare("UPDATE trans SET comment = ? WHERE id_relate = ?");
    q.addBindValue(transactionComment);
    q.addBindValue(pk_uid);
    if (!q.exec()) return false;

    return true;
}

bool TransactionsModel::setAmount(int pk_uid, double &transactionAmount)
{
    QSqlQuery q;

    //first update the selected transaction
    q.prepare("UPDATE trans SET amount = ? WHERE pk_uid = ?");
    q.addBindValue(transactionAmount);
    q.addBindValue(pk_uid);
    if (!q.exec()) return false;

    //next update the related transaction (if exists)
    q.clear();
    q.prepare("UPDATE trans SET amount = ? WHERE id_relate = ?");
    q.addBindValue(-1 * transactionAmount);
    q.addBindValue(pk_uid);
    if (!q.exec()) return false;

    return true;
}

bool TransactionsModel::setReconcile(int pk_uid, bool reconcileState)
{
    QSqlQuery q;

    if(reconcileState)  //reconciled should be set to true
    {
        q.prepare("UPDATE trans SET reconciled = 1 WHERE pk_uid = ?");
    }
    else                //reconciled should be set to false
    {
        q.prepare("UPDATE trans SET reconciled = 0 WHERE pk_uid = ?");
    }
    q.addBindValue(pk_uid);
    if(!q.exec()) return false;
    return true;
}

bool TransactionsModel::addTransaction(int &accountId, QString &transactionDate,QString &transactionComment,double &transactionAmount)
{
    QSqlQuery q;
    q.prepare("INSERT INTO trans (id_account, date_trans, comment, amount, reconciled) VALUES (?,?,?,?,0)");
    q.addBindValue(accountId);
    q.addBindValue(transactionDate);
    q.addBindValue(transactionComment);
    q.addBindValue(transactionAmount);
    return q.exec();
}

bool TransactionsModel::addTransactionRelation(int &transactionId, int &relateId)
{
    QSqlQuery q;
    q.prepare("UPDATE trans SET id_relate=? WHERE pk_uid=?");
    q.addBindValue(relateId);
    q.addBindValue(transactionId);
    return q.exec();
}

bool TransactionsModel::deleteTransaction(int &transactionId)
{
    QSqlQuery q;
    q.prepare("DELETE FROM trans WHERE pk_uid=? OR id_relate=?");
    q.addBindValue(transactionId);
    q.addBindValue(transactionId);
    return q.exec();
}

bool TransactionsModel::moveTransaction(int &accountId, int &transactionId)
{
    QSqlQuery updateQuery;
    updateQuery.prepare("UPDATE trans SET id_account=? WHERE pk_uid =?");
    updateQuery.addBindValue(accountId);
    updateQuery.addBindValue(transactionId);
    return updateQuery.exec();
}

QVariant TransactionsModel::data(const QModelIndex &item, int role) const
{
    QVariant d = QSqlQueryModel::data(item, role);
    if (item.column() == col_amount || item.column() == col_total)  //check for currency column
    {
        if(role == Qt::TextAlignmentRole)
        {
            return Qt::AlignRight;
        }
        else if (role == Qt::DisplayRole)
        {
            QLocale us(QLocale::English,QLocale::UnitedStates);
            return QVariant(us.toCurrencyString(d.toDouble(),us.currencySymbol()));
        }
        else
        {
            return d;
        }
    }
    else if (item.column() == col_comment && role == Qt::DisplayRole)  //check for comment (to add transfer information)
    {
        QModelIndex xferAccountIndex = QSqlQueryModel::index(item.row(),col_relate_account);
        QString xferAccountName = data(xferAccountIndex,role).toString();
        if (xferAccountName.length() > 0)
        {
            QString s = "Transfer (";
            s.append(xferAccountName);
            s.append("): ");
            s.append(d.toString());
            return QVariant(s);
        }
        return d;
    }
    else
    {
        return d;
    }
}




