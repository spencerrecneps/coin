#include "dialognewaccount.h"
#include "ui_dialognewaccount.h"

DialogNewAccount::DialogNewAccount(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DialogNewAccount)
{
    ui->setupUi(this);
}

DialogNewAccount::~DialogNewAccount()
{
    delete ui;
}
