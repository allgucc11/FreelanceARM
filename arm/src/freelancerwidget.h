#ifndef FREELANCERWIDGET_H
#define FREELANCERWIDGET_H

#include "models.h"
#include <QWidget>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QTextEdit>
#include <QTextBrowser>
#include <QListWidget>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QTimeEdit>
#include <QCheckBox>

class QUrl;

class FreelancerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit FreelancerWidget(const UserAccount &user, QWidget *parent = 0);

private slots:
    void choosePhoto();
    void saveProfile();
    void clearFields();
    void deleteProfileForm();
    void refreshBuyers();
    void onBuyerSelected();
    void chooseAttachment();
    void sendMessage();
    void refreshChat();
    void openChatLink(const QUrl &url);

private:
    QString selectedBuyerLogin() const;
    void loadProfile();
    void updatePhotoPreview();
    QString selectedWorkTime() const;
    void setWorkTimeFromString(const QString &value);

    UserAccount m_user;
    QLineEdit *m_nameEdit;
    QLineEdit *m_skillsEdit;
    QDoubleSpinBox *m_rateSpin;
    QLabel *m_ratingLabel;
    QLabel *m_completedLabel;
    QTimeEdit *m_workStartEdit;
    QTimeEdit *m_workEndEdit;
    QCheckBox *m_anyTimeCheck;
    QTextEdit *m_descriptionEdit;
    QLabel *m_photoLabel;
    QString m_photoPath;
    QListWidget *m_buyersList;
    QTextBrowser *m_chatHistory;
    QLineEdit *m_messageEdit;
    QPushButton *m_attachBtn;
    QLabel *m_attachmentLabel;
    QLabel *m_chatTitle;
    QTimer *m_timer;
    QString m_pendingAttachmentPath;
    QString m_pendingAttachmentName;
};

#endif
