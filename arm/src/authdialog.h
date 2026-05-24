#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include "models.h"
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>

class AuthDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AuthDialog(QWidget *parent = 0);
    UserAccount user() const;

private slots:
    void onLogin();
    void onRegister();

private:
    QLineEdit *m_loginEdit;
    QLineEdit *m_passwordEdit;
    QComboBox *m_roleBox;
    UserAccount m_user;
};

#endif
