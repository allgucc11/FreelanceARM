#ifndef BUYERWIDGET_H
#define BUYERWIDGET_H

#include "models.h"
#include <QWidget>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QListWidget>
#include <QTextBrowser>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QTimeEdit>
#include <QCheckBox>
class QVBoxLayout;
class QFrame;
class QUrl;

class BuyerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BuyerWidget(const UserAccount &user, QWidget *parent = 0);

private slots:
    void saveTaskAndRefresh();
    void refreshFreelancers();
    void onFreelancerSelected();
    void startOrder();
    void completeOrder();
    void chooseAttachment();
    void sendMessage();
    void refreshChat();
    void openChatLink(const QUrl &url);
    void rebuildCurrentCard();
    void delayedRefreshFreelancers();

private:
    QLabel *makePhotoLabel(const QString &path, int w, int h);
    QString selectedFreelancerLogin() const;
    void updateFreelancerCard(const FreelancerProfile &p);
    void showNoFreelancerSelected();
    void showChatLocked(const QString &text);
    void showChatArea(bool inputEnabled = true);
    void setChatVisible(bool visible);
    void setOrderControlsVisible(bool visible);
    void setPhoto(QLabel *label, const QString &path, int w, int h);
    bool matchesTask(const FreelancerProfile &p) const;
    QString currentTaskTitle() const;
    QString selectedTaskTimeRange() const;
    void resetCardColors();

    UserAccount m_user;
    QLineEdit *m_titleEdit;
    QLineEdit *m_skillEdit;
    QDoubleSpinBox *m_budgetSpin;
    QTimeEdit *m_taskStartTimeEdit;
    QTimeEdit *m_taskEndTimeEdit;
    QCheckBox *m_taskAnyTimeCheck;
    QListWidget *m_freelancerList;
    QWidget *m_cardWidget;
    QVBoxLayout *m_cardLayout;
    QTextBrowser *m_chatHistory;
    QLineEdit *m_messageEdit;
    QPushButton *m_sendBtn;
    QPushButton *m_attachBtn;
    QLabel *m_attachmentLabel;
    QTimer *m_timer;
    bool m_rebuildingCard;

    QLabel *m_placeholderLabel;
    QFrame *m_infoFrame;
    QLabel *m_selectedPhotoLabel;
    QLabel *m_selectedInfoLabel;
    QLabel *m_selfLabel;
    QWidget *m_orderWidget;
    QPushButton *m_startBtn;
    QPushButton *m_finishBtn;
    QLabel *m_lockedLabel;
    QLabel *m_chatTitle;
    QWidget *m_chatWidget;
    QString m_selectedLogin;
    QString m_pendingAttachmentPath;
    QString m_pendingAttachmentName;
};

#endif
