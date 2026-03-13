#ifndef ICOMMUNICATION_H
#define ICOMMUNICATION_H
/*
 * 通讯接口统一封装
*/
#include <QObject>
#include <QByteArray>
#include <QString>
#include "CommunicationParam.h"

class ICommunication : public QObject {
    Q_OBJECT

public:
    explicit ICommunication(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~ICommunication() {}

    // 连接设备
    virtual bool connectDevice(const CommunicationParam &param) = 0;
    // 断开设备连接
    virtual void disconnectDevice() = 0;
    // 发送数据
    virtual bool sendData(const QByteArray &data) = 0;
    // 获取设备连接状态
    virtual bool isConnected() const = 0;
    // 获取设备 ID
    virtual QString deviceId() const = 0;
    // 设置设备 ID
    virtual void setDeviceId(const QString &deviceId) = 0;

signals:
    // 数据接收信号
    void dataReceived(const QString &deviceId, const QByteArray &data);
    // 连接状态变化信号
    void connectionStateChanged(const QString &deviceId, bool isConnected);
    // 错误信号
    void errorOccurred(const QString &deviceId, const QString &errorString);
};

#endif // ICOMMUNICATION_H
