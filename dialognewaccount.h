#ifndef DIALOGNEWACCOUNT_H
#define DIALOGNEWACCOUNT_H

#include <QDialog>

namespace Ui {
class DialogNewAccount;
}

class DialogNewAccount : public QDialog
{
    Q_OBJECT
    
public:
    explicit DialogNewAccount(QWidget *parent = 0);
    ~DialogNewAccount();
    
private:
    Ui::DialogNewAccount *ui;
};

#endif // DIALOGNEWACCOUNT_H
