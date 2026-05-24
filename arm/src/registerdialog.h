#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QString>

class RegisterDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RegisterDialog(QWidget *parent = 0);

private slots:
    void choosePhoto();
    void onRegister();

private:
    void updatePhotoPreview();

    QLineEdit *m_nameEdit;
    QLineEdit *m_loginEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_repeatEdit;
    QLabel *m_photoLabel;
    QString m_photoPath;
};

#endif
