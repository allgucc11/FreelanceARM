#ifndef STORAGE_H
#define STORAGE_H

#include "models.h"
#include <QString>
#include <QVector>
#include <QStringList>
#include <QJsonArray>

class Storage
{
public:
    static QString dataDir();
    static QString copyAttachmentToStorage(const QString &sourcePath);
    static QString hashPassword(const QString &password);

    static QVector<UserAccount> loadUsers();
    static bool saveUsers(const QVector<UserAccount> &users);
    static bool loginExists(const QString &login);
    static bool addUser(const UserAccount &user, QString *errorText = 0);
    static bool authenticate(const QString &login, const QString &password, UserAccount *outUser, QString *errorText = 0);
    static QString displayNameForLogin(const QString &login);
    static QString photoForLogin(const QString &login);
    static bool updateUserPhoto(const QString &login, const QString &photoPath);
    static bool isUserDeleted(const QString &login);
    static bool deleteUserAccount(const QString &login, const QString &password, QString *errorText = 0);

    static QVector<FreelancerProfile> loadFreelancers();
    static bool saveFreelancers(const QVector<FreelancerProfile> &items);
    static bool saveFreelancerProfile(const FreelancerProfile &profile);
    static bool getFreelancerProfile(const QString &login, FreelancerProfile *profile);
    static bool deleteFreelancerProfile(const QString &login);
    static void updateFreelancerRating(const QString &freelancerLogin);

    static QVector<TaskInfo> loadTasks();
    static bool saveTasks(const QVector<TaskInfo> &items);
    static bool addTask(const TaskInfo &task);

    static QVector<OrderInfo> loadOrders();
    static bool saveOrders(const QVector<OrderInfo> &items);
    static bool startOrder(const QString &buyerLogin, const QString &freelancerLogin, const QString &taskTitle);
    static bool completeOrder(const QString &buyerLogin, const QString &freelancerLogin, int rating);
    static bool hasStartedOrder(const QString &buyerLogin, const QString &freelancerLogin);
    static bool hasActiveOrder(const QString &buyerLogin, const QString &freelancerLogin);
    static bool hasCompletedRatedOrder(const QString &buyerLogin, const QString &freelancerLogin);
    static QString activeOrderId(const QString &buyerLogin, const QString &freelancerLogin);

    static QVector<ChatMessage> loadMessages();
    static bool saveMessages(const QVector<ChatMessage> &items);
    static bool addMessage(const ChatMessage &message);
    static bool addSystemMessage(const QString &orderId, const QString &buyerLogin, const QString &freelancerLogin, const QString &text);
    static QString latestOrderId(const QString &buyerLogin, const QString &freelancerLogin);
    static QVector<ChatMessage> messagesForOrder(const QString &orderId);
    static QVector<ChatMessage> messagesForPair(const QString &buyerLogin, const QString &freelancerLogin);
    static QStringList buyersForFreelancer(const QString &freelancerLogin);

private:
    static bool ensureDataDir();
    static bool readArray(const QString &fileName, QJsonArray *array);
    static bool writeArray(const QString &fileName, const QJsonArray &array);
};

#endif
