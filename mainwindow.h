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

void refreshAccountTree();

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_btnAccept_clicked();
    void on_treeAccounts_itemSelectionChanged();
    int getAccountId();
    int getTransactionId();
    int getTransactionId(int rowNum);
    void on_tableTransactions_customContextMenuRequested(const QPoint &pos);
    void on_transferCheckBox_stateChanged(int arg1);
    void fillAccountCombo();
    void on_lineEditFilter_textChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;
    QSqlDatabase db;
    QStandardItemModel *accountsTree;
    TransactionsModel *transactions;
    QSortFilterProxyModel *accountFilter;
    QSortFilterProxyModel *commentFilter;
    void transactionFailedError();
};

#endif // MAINWINDOW_H
