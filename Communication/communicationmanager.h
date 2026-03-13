#ifndef COMMUNICATIONMANAGER_H
#define COMMUNICATIONMANAGER_H

#include <QObject>

#include <QObject>
#include <QMap>
#include <QThread>
#include "ICommunication.h"
#include "CommunicationParam.h"

// 设备信息结构体，包含通信对象和线程
struct Device {
    ICommunication *communication;
    QThread *thread;
    DeviceInfo deviceInfo;
};

class CommunicationManager : public QObject {
    Q_OBJECT

public:
    explicit CommunicationManager(QObject *parent = nullptr);
    ~CommunicationManager() override;

    // 添加设备
    bool addDevice(const DeviceInfo &deviceInfo);
    // 移除设备
    void removeDevice(const QString &deviceId);
    // 连接设备
    bool connectDevice(const QString &deviceId);
    // 断开设备连接
    void disconnectDevice(const QString &deviceId);
    // 发送数据
    bool sendData(const QString &deviceId, const QByteArray &data);
    // 获取设备连接状态
    bool isDeviceConnected(const QString &deviceId) const;
    // 获取所有设备 ID
    QStringList allDeviceIds() const;

signals:
    // 数据接收信号
    void dataReceived(const QString &deviceId, const QByteArray &data);
    // 设备连接状态变化信号
    void deviceConnectionStateChanged(const QString &deviceId, bool isConnected);
    // 设备错误信号
    void deviceErrorOccurred(const QString &deviceId, const QString &errorString);

private slots:
    // 转发通信对象的数据接收信号
    void onDataReceived(const QString &deviceId, const QByteArray &data);
    // 转发通信对象的连接状态变化信号
    void onConnectionStateChanged(const QString &deviceId, bool isConnected);
    // 转发通信对象的错误信号
    void onErrorOccurred(const QString &deviceId, const QString &errorString);

private:


    // 设备映射表，key 为设备 ID
    QMap<QString, Device> m_devices;
};

#endif // COMMUNICATIONMANAGER_H

