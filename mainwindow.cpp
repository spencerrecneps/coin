#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QMessageBox"
#include "QtDebug"

//#define pathDB "/shared/coin/coin.db"
#define pathDB "/home/spencer/dev/coin/coin.db"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    //set the ui
    ui->setupUi(this);
    ui->dateEdit->setDate(QDate::currentDate());  //set the date to today
    ui->lineEditAmount->setValidator(new QDoubleValidator(-INFINITY,INFINITY,2));  //force 2-decimal number in amount field
    ui->comboAccounts->hide();  //hide the transfer to account combobox

    //set the db and open
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(pathDB);
    QFileInfo checkFile(pathDB);
    if(!checkFile.isFile() or !db.open())
    {
        QMessageBox::critical(0, qApp->tr("Cannot open database"),
            qApp->tr("Unable to establish a database connection.\n"
                     "Click Cancel to exit."), QMessageBox::Cancel);
        QApplication::quit();
    }

    //establish accounts and select first account
    refreshAccountTree();
    ui->treeAccounts->expandAll();

    //set up transactions table
    transactions = new TransactionsModel(this);
    transactions->refresh();

    //set up the filter
    filteredTransactions = new QSortFilterProxyModel(this);
    filteredTransactions->setDynamicSortFilter(true);
    filteredTransactions->setSourceModel(transactions);

    if (ui->treeAccounts->selectedItems().count() == 1)
    {
        filteredTransactions->setFilterKeyColumn(1);
        filteredTransactions->setFilterFixedString(QString::number(getAccountId()));
    }

    ui->tableTransactions->setModel(filteredTransactions);

    //hide the pk_uid, id_account, and related account columns
    ui->tableTransactions->hideColumn(0);
    ui->tableTransactions->hideColumn(1);
    ui->tableTransactions->hideColumn(2);

    //NEED TO CREATE DELEGATE FOR TABLE CELL FORMATTING
    //SEE http://www.youtube.com/watch?v=EJf-vZ6FQfc
    //SEE http://qt-project.org/doc/qt-4.8/qstyleditemdelegate.html


    //select the first account (so that there is a selection active)
    ui->treeAccounts->setCurrentItem(ui->treeAccounts->itemAt(0,0));
}

MainWindow::~MainWindow()
{
    delete ui;
    db.close();
}

void MainWindow::refreshAccountTree()
{
    int previousSelectionId = getAccountId();  //save the currently-selected pk_uid. if no selection, returns -1
    ui->treeAccounts->clear();

    QSqlQuery *parentAccounts;
    QSqlQuery *childAccounts;

    //get the parent accounts
    parentAccounts = new QSqlQuery();
    parentAccounts->exec("SELECT pk_uid, account_name FROM account WHERE id_parent IS NULL ORDER BY account_name");

    while(parentAccounts->next())
    {
        QTreeWidgetItem *itmParent;
        itmParent = new QTreeWidgetItem();
        itmParent->setText(0,parentAccounts->value(1).toString());
        itmParent->setData(0,Qt::UserRole,parentAccounts->value(0).toInt());

        //add the parent entry
        ui->treeAccounts->addTopLevelItem(itmParent);

        //get the child accounts
        childAccounts = new QSqlQuery();
        childAccounts->prepare("SELECT pk_uid, account_name FROM account WHERE id_parent=? ORDER BY account_name");
        childAccounts->addBindValue(parentAccounts->value(0));
        childAccounts->exec();

        while(childAccounts->next())
        {
            QTreeWidgetItem *itmChild;
            itmChild = new QTreeWidgetItem();
            itmChild->setText(0,childAccounts->value(1).toString());
            itmChild->setData(0,Qt::UserRole,childAccounts->value(0).toInt());
            itmParent->addChild(itmChild);
        }
    }
}

void MainWindow::on_btnAccept_clicked()
{
    double transactionAmount, transferAmount;
    int accountId, transferAccountId, firstTransactionId, secondTransactionId;
    QString transactionComment, transactionDate;

    //get the account id and the information from the fields
    accountId = getAccountId();
    transferAccountId = ui->comboAccounts->itemData(ui->comboAccounts->currentIndex()).toInt();
    transactionAmount = ui->lineEditAmount->text().toDouble();
    transferAmount = transactionAmount*-1;
    transactionComment = ui->lineEditTransactionInfo->text();
    transactionDate = ui->dateEdit->date().toString("yyyy-MM-dd");

    //check to see if this is a transfer
    if (ui->transferCheckBox->isChecked())  //this is a transfer
    {
        QSqlQuery q;

        //perform the first part of the transfer. if it fails, kick out an error message and exit routine
        if(!transactions->addTransaction(accountId,transactionDate,transactionComment,transactionAmount))
        {
            QMessageBox::critical(0, qApp->tr("Transaction Error"),
                qApp->tr("Could not add transaction."), QMessageBox::Cancel);
            return;
        }

        //get the id of the first part of the transfer
        q.exec("SELECT last_insert_rowid()");
        q.first();
        firstTransactionId = q.value(0).toInt();

        //perform the second part of the transfer. if it fails, kick out an error message and exit routine
        if(!transactions->addTransaction(transferAccountId,transactionDate,transactionComment,transferAmount))
        {
            QMessageBox::critical(0, qApp->tr("Transaction Error"),
                qApp->tr("Could not add transaction."), QMessageBox::Cancel);
            return;
        }

        //get the id of the second part of the transfer
        q.clear();
        q.exec("SELECT last_insert_rowid()");
        q.first();
        q.value(0);
        secondTransactionId = q.value(0).toInt();

        if(!transactions->addTransactionRelation(firstTransactionId,secondTransactionId))
        {
            QMessageBox::critical(0, qApp->tr("Transaction Error"),
                qApp->tr("Could not add transaction."), QMessageBox::Cancel);
            return;
        }

        if(!transactions->addTransactionRelation(secondTransactionId,firstTransactionId))
        {
            QMessageBox::critical(0, qApp->tr("Transaction Error"),
                qApp->tr("Could not add transaction."), QMessageBox::Cancel);
            return;
        }
    }
    else  //this is not a transfer
    {
        //perform the transaction. if it fails, kick out an error message
        if(!transactions->addTransaction(accountId,transactionDate,transactionComment,transactionAmount))
        {
            QMessageBox::critical(0, qApp->tr("Transaction Error"),
                qApp->tr("Could not add transaction."), QMessageBox::Cancel);
        }
    }

    //clear the amount and comment lines
    ui->lineEditAmount->clear();
    ui->lineEditTransactionInfo->clear();

    //refresh the table
    transactions->refresh();
}

void MainWindow::on_treeAccounts_itemSelectionChanged()
{
    transactions->refresh();
    filteredTransactions->setFilterKeyColumn(1);
    filteredTransactions->setFilterFixedString(QString::number(getAccountId()));

    //if the transfer combobox is showing, update the accounts to reflect the change
    if (ui->transferCheckBox->checkState() == Qt::Checked)
    {
        ui->comboAccounts->clear();
        fillAccountCombo();
    }
}

/*
 *  returns the pk_uid of the account selected in the account tree view
 */
int MainWindow::getAccountId()
{
    if (ui->treeAccounts->selectedItems().count() == 0)
    {
        return -1;
    }
    else
    {
        return ui->treeAccounts->selectedItems().first()->data(0,Qt::UserRole).toInt();
    }
}

/*
 *  returns the pk_uid of the transaction selected in the transactions table
 */
int MainWindow::getTransactionId()
{
    int rowId;
    int transactionId;

    rowId = ui->tableTransactions->selectionModel()->currentIndex().row();
    transactionId = ui->tableTransactions->model()->data(ui->tableTransactions->model()->index(rowId,0)).toInt();

    return transactionId;
}

/*
 *  right click menu for transactions
 */
void MainWindow::on_tableTransactions_customContextMenuRequested(const QPoint &pos)
{
    //test for a click in empty table space, or for no selection
    if (ui->tableTransactions->selectionModel()->selectedRows().count() != 1 || !ui->tableTransactions->indexAt(pos).isValid())
    {
        return;
    }

    QMenu *transactionsMenu;
    QPoint globalPos = ui->tableTransactions->mapToGlobal(pos);
    QMenu *accountsMenu;
    QSqlQuery q;

    //create the menus
    transactionsMenu = new QMenu(this);
    accountsMenu = new QMenu("Move transaction",transactionsMenu);
    transactionsMenu->addMenu(accountsMenu);

    //add the delete transaction action
    QAction *deleteAction;
    deleteAction = new QAction("Delete this transaction",transactionsMenu);
    deleteAction->setData("delete");
    transactionsMenu->addAction(deleteAction);

    //query the accounts
    q.prepare("SELECT pk_uid, account_name FROM account WHERE pk_uid <> ? ORDER BY account_name");
    q.addBindValue(QString::number(getAccountId()));
    q.exec();

    //iterate through the accounts selected in the sql query, create an action for each, and populate the submenu
    while (q.next())
    {
        QAction *a;
        a = new QAction(q.value(1).toString(),accountsMenu);
        a->setData(q.value(0).toInt());  //need to store the pk_uid for each account with the menu item
        accountsMenu->addAction(a);
    }

    //show the menu
    QAction *selectedItem = transactionsMenu->exec(globalPos);
    if (selectedItem)
    {
        int accountId = selectedItem->data().toInt();
        int transactionId = getTransactionId();

        if(selectedItem->data() == "delete")  //if the user clicked on "delete this transaction"
        {
            QSqlQuery deleteQuery;
            deleteQuery.prepare("DELETE FROM trans WHERE pk_uid=? OR id_relate=?");
            deleteQuery.addBindValue(transactionId);
            deleteQuery.addBindValue(transactionId);
            deleteQuery.exec();
        }
        else //the user selected an account to move the transaction to
        {
            QSqlQuery updateQuery;
            updateQuery.prepare("UPDATE trans SET id_account=? WHERE pk_uid=?");
            updateQuery.addBindValue(accountId);
            updateQuery.addBindValue(transactionId);
            updateQuery.exec();
        }

        transactions->refresh();
    }
}

/*
 *  toggle checkbox for a transfer
 */
void MainWindow::on_transferCheckBox_stateChanged(int arg1)
{
    if (arg1 == 0)  //if the user unchecked the transfer
    {
        ui->comboAccounts->clear();
        ui->comboAccounts->hide();
    }
    else  //if the user checked for a transfer
    {
        fillAccountCombo();
        ui->comboAccounts->show();
    }
}

/*
 *  fills the account combobox with accounts for a transfer transaction
 */
void MainWindow::fillAccountCombo()
{
    //query the accounts
    QSqlQuery q;
    q.prepare("SELECT pk_uid, account_name FROM account WHERE pk_uid <> ? ORDER BY account_name");
    q.addBindValue(QString::number(getAccountId()));
    q.exec();

    //add accounts to the combobox
    while(q.next())
    {
        ui->comboAccounts->addItem(q.value(1).toString(),q.value(0));
    }
}

