#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "QMessageBox"
#include "QtDebug"
#include "definitions.h"

//#define pathDB "/shared/coin/coin.db"
#define pathDB "/home/spencer/dev/coin/coin/coin.db"

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

    //set up the filters
    accountFilter = new QSortFilterProxyModel(this);
    reconcileFilter = new QSortFilterProxyModel(this);
    commentFilter = new QSortFilterProxyModel(this);
    accountFilter->setFilterKeyColumn(col_id_account);
    reconcileFilter->setFilterKeyColumn(col_reconciled);
    commentFilter->setFilterKeyColumn(col_comment);
    accountFilter->setDynamicSortFilter(true);
    reconcileFilter->setDynamicSortFilter(true);
    commentFilter->setDynamicSortFilter(true);
    accountFilter->setSourceModel(transactions);
    reconcileFilter->setSourceModel(accountFilter);
    commentFilter->setSourceModel(reconcileFilter);

    if (ui->treeAccounts->selectedItems().count() == 1)     //filter the table based on the selection in the accounts tree
    {
        accountFilter->setFilterFixedString(QString::number(getAccountId()));
    }
    reconcileFilter->setFilterFixedString(QString::number(0));             //filter the table for unreconciled transactions only
    ui->tableTransactions->setModel(commentFilter);         //filter the table for tag searches in the box

    //hide the pk_uid, id_account, and related account columns
    //and size the remaining columns appropriately
    ui->tableTransactions->hideColumn(col_pk_uid);
    ui->tableTransactions->hideColumn(col_id_account);
    ui->tableTransactions->hideColumn(col_relate_account);
    ui->tableTransactions->hideColumn(col_reconciled);
    QHeaderView *h = ui->tableTransactions->horizontalHeader();
    h->setStretchLastSection(false);
    h->setSectionResizeMode(col_date,QHeaderView::Fixed);
    h->setSectionResizeMode(col_comment,QHeaderView::Stretch);  //make the comments column stretch to fill leftover space
    h->setSectionResizeMode(col_amount,QHeaderView::Fixed);
    h->setSectionResizeMode(col_total,QHeaderView::Fixed);
    h->resizeSection(col_date,100);
    h->resizeSection(col_amount,100);
    h->resizeSection(col_total,120);

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
//    int previousSelectionId = getAccountId();  //save the currently-selected pk_uid. if no selection, returns -1
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
            transactionFailedError(qApp->tr("Could not add transaction."));
            return;
        }

        //get the id of the first part of the transfer
        q.exec("SELECT last_insert_rowid()");
        q.first();
        firstTransactionId = q.value(0).toInt();

        //perform the second part of the transfer. if it fails, kick out an error message and exit routine
        if(!transactions->addTransaction(transferAccountId,transactionDate,transactionComment,transferAmount))
        {
            transactionFailedError(qApp->tr("Could not add transaction."));
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
            transactionFailedError(qApp->tr("Could not add transaction."));
            return;
        }

        if(!transactions->addTransactionRelation(secondTransactionId,firstTransactionId))
        {
            transactionFailedError(qApp->tr("Could not add transaction."));
            return;
        }
    }
    else  //this is not a transfer
    {
        //perform the transaction. if it fails, kick out an error message
        if(!transactions->addTransaction(accountId,transactionDate,transactionComment,transactionAmount))
        {
            transactionFailedError(qApp->tr("Could not add transaction."));
            return;
        }
    }

    //clear the amount and comment lines
    ui->lineEditAmount->clear();
    ui->lineEditTransactionInfo->clear();

    //clear the transfer checkbox and hide the combobox
    ui->transferCheckBox->setChecked(false);
    ui->comboAccounts->clear();
    ui->comboAccounts->hide();

    //refresh the table
    transactions->refresh();

    //scroll to the bottom
    //ui->tableTransactions->scrollToBottom();
}

void MainWindow::on_treeAccounts_itemSelectionChanged()
{
    transactions->refresh();
    accountFilter->setFilterFixedString(QString::number(getAccountId()));

    //if the transfer combobox is showing, update the accounts to reflect the change
    if (ui->transferCheckBox->checkState() == Qt::Checked)
    {
        ui->comboAccounts->clear();
        fillAccountCombo();
    }

    //scroll to the bottom of the transactions
    //ui->tableTransactions->scrollToBottom();
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

QString MainWindow::getAccountName()
{
    return ui->treeAccounts->selectedItems().first()->data(0,Qt::DisplayRole).toString();
}

/*
 *  returns the pk_uid of the transaction selected in the transactions table
 */
int MainWindow::getTransactionId()
{
    int rowId;
    rowId = ui->tableTransactions->selectionModel()->currentIndex().row();
    return getTransactionId(rowId);
}

/*
 *  returns the pk_uid of the transaction at the given row position in the transactions table
 */
int MainWindow::getTransactionId(int rowNum)
{
    int transactionId;
    transactionId = ui->tableTransactions->model()->data(ui->tableTransactions->model()->index(rowNum,col_pk_uid)).toInt();
    return transactionId;
}

/*
 *  right click menu for transactions
 */
void MainWindow::on_tableTransactions_customContextMenuRequested(const QPoint &pos)
{
    //test for a click in empty table space, or for no selection
    if (ui->tableTransactions->selectionModel()->selectedRows().count() < 1 || !ui->tableTransactions->indexAt(pos).isValid())
    {
        return;
    }

    QMenu *transactionsMenu;
    QPoint globalPos = ui->tableTransactions->mapToGlobal(pos);
    QMenu *accountsMenu;
    QSqlQuery q;
    QString moveText;
    QString deleteText;
    QString reconcileText;

    //set the menu item text based on the number of selected transactions
    if (ui->tableTransactions->selectionModel()->selectedRows().count() == 1)
    {
        moveText = "Move transaction";
        deleteText = "Delete this transaction";
    }
    else
    {
        moveText = "Move transactions";
        deleteText = "Delete these transactions";
    }
    reconcileText = "Mark as reconciled";  //not dependent on number of transactions selected

    //create the menus
    transactionsMenu = new QMenu(this);
    accountsMenu = new QMenu(moveText,transactionsMenu);
    transactionsMenu->addMenu(accountsMenu);

    //add the delete transaction action
    QAction *deleteAction;
    deleteAction = new QAction(deleteText,transactionsMenu);
    deleteAction->setData("delete");
    transactionsMenu->addAction(deleteAction);

    //add the reconcile action
    QAction *reconcileAction;
    reconcileAction = new QAction(reconcileText,transactionsMenu);
    reconcileAction->setData("reconcile");
    transactionsMenu->addAction(reconcileAction);

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

    //show the menu and get the selected menu item
    QAction *selectedMenuItem = transactionsMenu->exec(globalPos);

    if (selectedMenuItem)
    {
        QItemSelectionModel *rowsSelectionModel;
        QModelIndexList rowsList;

        //set up the selection model and get the pk_uid column of the selected rows
        rowsSelectionModel = ui->tableTransactions->selectionModel();
        rowsList = rowsSelectionModel->selectedRows(col_pk_uid);

        //iterate through the selection and perform the selected task on each
        //selected transaction
        QList<QModelIndex>::Iterator i;
        for (i = rowsList.begin(); i != rowsList.end(); ++i)
        {
            int transactionId;
            transactionId = i->data().toInt();

            if(selectedMenuItem->data() == "delete")  //if the user clicked on "delete this transaction"
            {
                if(!transactions->deleteTransaction(transactionId))
                {
                    transactionFailedError(qApp->tr("Could not delete transaction."));
                    return;
                }
            }
            else if(selectedMenuItem->data() == "reconcile")
            {
                if(!transactions->setReconcile(transactionId,true))
                {
                    transactionFailedError(qApp->tr("Could not set as reconciled."));
                    return;
                }
            }
            else    //the user selected an account to move the transaction to
            {
                int accountId = selectedMenuItem->data().toInt();

                if (!transactions->moveTransaction(accountId, transactionId))
                {
                    transactionFailedError(qApp->tr("Could not move transaction."));
                    return;
                }
            }
        }

        transactions->refresh();
        //ui->tableTransactions->scrollToBottom();
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

/*
 *  sets a word filter on transactions
 */
void MainWindow::on_lineEditFilter_textChanged(const QString &arg1)
{
    commentFilter->setFilterRegExp(arg1);
}

void MainWindow::transactionFailedError(QString errMessage)
{
    QMessageBox::critical(0, qApp->tr("Transaction Error"),
        errMessage, QMessageBox::Cancel);
    return;
}

void MainWindow::on_btnAddAccount_clicked()
{

}

void MainWindow::on_btnDeleteAccount_clicked()
{
    QString selectedAccountName = getAccountName();
    QString warningMessage = qApp->tr("This will delete account ");
    warningMessage.append(selectedAccountName);
    if (QMessageBox::warning(0,qApp->tr("Delete Account"),
                         warningMessage,
                         QMessageBox::Ok | QMessageBox::Cancel)
            == QMessageBox::Ok)
    {
        int accountId = getAccountId();

        //remove any transactions associated with this account
        QSqlQuery q;
        q.prepare("DELETE FROM trans WHERE id_account = ?");
        q.addBindValue(accountId);
        if(!q.exec())
        {
            transactionFailedError(qApp->tr("Could not delete account"));
        }

        //remove the account
        q.clear();
        q.prepare("DELETE FROM account WHERE pk_uid = ?");
        q.addBindValue(accountId);
        if(!q.exec())
        {
            transactionFailedError(qApp->tr("Could not delete account"));
        }
    }
    refreshAccountTree();
}
