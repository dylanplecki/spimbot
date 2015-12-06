/********************************************************************************
** Form generated from reading UI file 'spimbotwidget.ui'
**
** Created: Tue Jan 29 16:12:08 2013
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SPIMBOTWIDGET_H
#define UI_SPIMBOTWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SpimbotWidget
{
public:

    void setupUi(QWidget *SpimbotWidget)
    {
        if (SpimbotWidget->objectName().isEmpty())
            SpimbotWidget->setObjectName(QString::fromUtf8("SpimbotWidget"));
        //TODO Make window size easier to change
        SpimbotWidget->resize(350, 350);
        SpimbotWidget->setAutoFillBackground(true);

        retranslateUi(SpimbotWidget);

        QMetaObject::connectSlotsByName(SpimbotWidget);
    } // setupUi

    void retranslateUi(QWidget *SpimbotWidget)
    {
        SpimbotWidget->setWindowTitle(QApplication::translate("SpimbotWidget", "Form", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class SpimbotWidget: public Ui_SpimbotWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SPIMBOTWIDGET_H
