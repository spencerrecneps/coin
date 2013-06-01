#ifndef CURRENCYDELEGATE_H
#define CURRENCYDELEGATE_H

#include <QItemDelegate>
#include <QModelIndex>
#include <QObject>
#include <QSize>

class CurrencyDelegate : public QItemDelegate
{
    Q_OBJECT
public:
    explicit CurrencyDelegate(QObject *parent = 0);
    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void setEditorData(QWidget *editor, const QModelIndex &index) const;
    void setModelData(QWidget *editor, QAbstractItemModel *model, const QModelIndex &index) const;
    void updateEditorGeometry(QWidget *editor, const QStyleOptionViewItem &option, const QModelIndex &index) const;

signals:
    
public slots:
    
};

#endif // CURRENCYDELEGATE_H
