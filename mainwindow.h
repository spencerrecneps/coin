#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDebug>
#include "transactionsmodel.h"
#include <QtSql>
#include <QFileInfo>
#include <QtCore>
#include <QtGui>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btnAccept_clicked();
    void on_treeAccounts_itemSelectionChanged();
    void on_tableTransactions_customContextMenuRequested(const QPoint &pos);
    void on_transferCheckBox_stateChanged(int arg1);
    void on_lineEditFilter_textChanged(const QString &arg1);
    void on_btnAddAccount_clicked();
    void on_btnDeleteAccount_clicked();
    void on_actionReconciled_triggered(bool checked);

private:
    Ui::MainWindow *ui;
    QSqlDatabase db;
    QStandardItemModel *accountsTree;
    TransactionsModel *transactions;
    QSortFilterProxyModel *accountFilter;
    QSortFilterProxyModel *reconcileFilter;
    QSortFilterProxyModel *commentFilter;
    int getAccountId();
    QString getAccountName();
    int getTransactionId();
    int getTransactionId(int rowNum);
    void fillAccountCombo();
    void transactionFailedError(QString errMessage);
    void refreshAccountTree();
    float sumColumn(int column);
    void setFilterAmount();
};

#endif // MAINWINDOW_H
