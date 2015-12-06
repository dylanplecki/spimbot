/********************************************************************************
** Form generated from reading UI file 'spimbotview.ui'
**
** Created: Tue Jan 29 16:12:08 2013
**      by: Qt User Interface Compiler version 4.6.2
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_SPIMBOTVIEW_H
#define UI_SPIMBOTVIEW_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHeaderView>
#include <QtGui/QMainWindow>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_SpimbotView
{
public:
    QWidget *centralWidget;

    void setupUi(QMainWindow *SpimbotView)
    {
        if (SpimbotView->objectName().isEmpty())
            SpimbotView->setObjectName(QString::fromUtf8("SpimbotView"));
        //TODO Make window size easier to change
        SpimbotView->resize(350, 350);
        centralWidget = new QWidget(SpimbotView);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        SpimbotView->setCentralWidget(centralWidget);

        retranslateUi(SpimbotView);

        QMetaObject::connectSlotsByName(SpimbotView);
    } // setupUi

    void retranslateUi(QMainWindow *SpimbotView)
    {
        SpimbotView->setWindowTitle(QApplication::translate("SpimbotView", "Map", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class SpimbotView: public Ui_SpimbotView {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_SPIMBOTVIEW_H
