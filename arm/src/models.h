#ifndef MODELS_H
#define MODELS_H

#include <QString>
#include <QVector>
#include <QJsonObject>

struct UserAccount {
    QString login;
    QString displayName;
    QString role; // НЕ сохраняется в базе. Временная роль текущего входа: buyer или freelancer
    QString passwordHash;
    QString photoPath;
    int purchases;
    bool deleted;

    UserAccount() : purchases(0), deleted(false) {}
    QJsonObject toJson() const;
    static UserAccount fromJson(const QJsonObject &obj);
};

struct FreelancerProfile {
    QString login;
    QString name;
    QString skills;
    double rate;
    double rating;
    QString workTime;
    QString description;
    QString photoPath;
    int completedOrders;

    FreelancerProfile() : rate(0.0), rating(0.0), completedOrders(0) {}
    QJsonObject toJson() const;
    static FreelancerProfile fromJson(const QJsonObject &obj);
};

struct TaskInfo {
    QString buyerLogin;
    QString title;
    QString skill;
    double budget;
    QString timeRange;

    TaskInfo() : budget(0.0) {}
    QJsonObject toJson() const;
    static TaskInfo fromJson(const QJsonObject &obj);
};

struct OrderInfo {
    QString orderId;
    QString buyerLogin;
    QString freelancerLogin;
    QString taskTitle;
    QString status; // in_progress или completed
    int rating;

    OrderInfo() : rating(0) {}
    QJsonObject toJson() const;
    static OrderInfo fromJson(const QJsonObject &obj);
};

struct ChatMessage {
    QString orderId;
    QString buyerLogin;
    QString freelancerLogin;
    QString senderLogin;
    QString senderName;
    QString text;
    QString time;
    QString attachmentName;
    QString attachmentPath;

    QJsonObject toJson() const;
    static ChatMessage fromJson(const QJsonObject &obj);
};

#endif
