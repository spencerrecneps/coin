#include <QtSql>
#include <QLocale>
#include "transactionsmodel.h"


TransactionsModel::TransactionsModel(QObject *parent) :
    QSqlQueryModel(parent)
{
}

Qt::ItemFlags TransactionsModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = QSqlQueryModel::flags(index);
    if (index.column() > 1 && index.column() < 5 )
        flags |= Qt::ItemIsEditable;
    return flags;
}

bool TransactionsModel::setData(const QModelIndex &index, const QVariant &value, int /*role*/)
{
    if (index.column() < 3 || index.column() == 6) {
        return false;
    }
    QModelIndex primaryKeyIndex = QSqlQueryModel::index(index.row(),0);
    int pk_uid = data(primaryKeyIndex, Qt::EditRole).toInt();

    clear();

    bool success;
    if (index.column() == 3) {
        success = setDate(pk_uid,value.toString());
    }
    else if (index.column() == 4) {
        success = setComment(pk_uid,value.toString());
    }
    else if (index.column() == 5) {
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
    setQuery("SELECT pk_uid, id_account, relate_account, date_trans, comment, amount, total FROM trans_total ORDER BY date_trans, pk_uid");
    setHeaderData(0,Qt::Horizontal,QObject::tr("pk_uid"));
    setHeaderData(1,Qt::Horizontal,QObject::tr("id_account"));
    setHeaderData(2,Qt::Horizontal,QObject::tr("relate_account"));
    setHeaderData(3,Qt::Horizontal,QObject::tr("Date"));
    setHeaderData(4,Qt::Horizontal,QObject::tr("Comment"));
    setHeaderData(5,Qt::Horizontal,QObject::tr("Amount"));
    setHeaderData(6,Qt::Horizontal,QObject::tr("Total"));
}

bool TransactionsModel::setDate(int pk_uid, const QString &transactionDate)
{
    QSqlQuery q;
    q.prepare("UPDATE trans SET date_trans = ? WHERE pk_uid = ?");
    q.addBindValue(transactionDate);
    q.addBindValue(pk_uid);
    return q.exec();
}

bool TransactionsModel::setComment(int pk_uid, const QString &transactionComment)
{
    QSqlQuery q;
    q.prepare("UPDATE trans SET comment = ? WHERE pk_uid = ?");
    q.addBindValue(transactionComment);
    q.addBindValue(pk_uid);
    return q.exec();
}

bool TransactionsModel::setAmount(int pk_uid, double &transactionAmount)
{
    bool success=false;
    QSqlQuery q;

    //first update the selected transaction
    q.prepare("UPDATE trans SET amount = ? WHERE pk_uid = ?");
    q.addBindValue(transactionAmount);
    q.addBindValue(pk_uid);
    success = q.exec();

    if (!success) return success;  //exit if the transaction failed

    //next update the related transaction (if exists)
    q.clear();
    q.prepare("UPDATE trans SET amount = ? WHERE id_relate = ?");
    q.addBindValue(-1 * transactionAmount);
    q.addBindValue(pk_uid);
    success = q.exec();

    return success;
}

bool TransactionsModel::addTransaction(int &accountId, QString &transactionDate,QString &transactionComment,double &transactionAmount)
{
    QSqlQuery q;
    q.prepare("INSERT INTO trans (id_account, date_trans, comment, amount) VALUES (?,?,?,?)");
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

QVariant TransactionsModel::data(const QModelIndex &item, int role) const
{
    QVariant d = QSqlQueryModel::data(item, role);
    if (item.column() == 5 || item.column() == 6)  //check for currency column
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
        /*
         *STILL NEED TO CONSIDER HOW TO ACCOMPLISH THIS, WE HAVE THE RELATED ACCOUNT
         *INCLUDED IN THE TABLE MODEL, BUT HOW TO ACCESS ITS DATA FROM HERE...
         *
         *else if (item.column() == 4)  //check for comment (to add transfer information)
         *{
         *    if (role == Qt::DisplayRole)
         *    {
         *        QString s = "Transfer (";
         *        s.append(d.toString());
         *        s.append(")");
         *        return QVariant(s);
         *    }
         *}
        */
        else
        {
            return d;
        }
    }
    else
    {
        return d;
    }
}




