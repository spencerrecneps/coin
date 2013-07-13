#ifndef TRANSACTIONSMODEL_H
#define TRANSACTIONSMODEL_H

#include <QSqlQueryModel>

class TransactionsModel : public QSqlQueryModel
{
    Q_OBJECT
public:
    explicit TransactionsModel(QObject *parent = 0);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool setData(const QModelIndex &index, const QVariant &value, int role);
    bool setReconcile(int pk_uid, bool reconcileState);
    bool addTransaction(int &accountId, QString &transactionDate,QString &transactionComment,double &transactionAmount);
    bool addTransactionRelation(int &transactionId, int &relateId);
    bool deleteTransaction(int &transactionId);
    bool moveTransaction(int &accountId, int &transactionId);
    void refresh();
    QVariant data(const QModelIndex &item, int role) const;

private:
    bool setDate(int pk_uid, const QString &transactionDate);
    bool setComment(int pk_uid, const QString &transactionComment);
    bool setAmount(int pk_uid, double &transactionAmount);
};

#endif // TRANSACTIONSMODEL_H
