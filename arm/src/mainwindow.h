#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "models.h"
#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);

private slots:
    void logout();
    void deleteProfile();
    void showProfileDialog();

private:
    void showAuth();
    void buildForUser(const UserAccount &user);
    void applyStyle();

    QWidget *m_central;
    QStackedWidget *m_stack;
    UserAccount m_user;
};

#endif
