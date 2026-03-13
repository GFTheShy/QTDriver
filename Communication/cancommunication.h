#ifndef CANCOMMUNICATION_H
#define CANCOMMUNICATION_H
/*
 * CAN通讯策略类
 * 实现CAN的功能底层逻辑
*/
#include <QObject>
#include "ICommunication.h"
#include <QCanBus>
#include <QCanBusDevice>

class CanCommunication : public ICommunication {
    Q_OBJECT

public:
    explicit CanCommunication(QObject *parent = nullptr);
    ~CanCommunication() override;
    // 连接CAN设备
    bool connectDevice(const CommunicationParam &param) override;
    // 断开CAN设备
    void disconnectDevice() override;
    // 发送数据
    bool sendData(const QByteArray &data) override;
    // 判断是否连接
    bool isConnected() const override;
    // 获取设备ID
    QString deviceId() const override;
    // 设置设备ID
    void setDeviceId(const QString &deviceId) override;

private slots:
    void onFramesReceived();
    void onErrorOccurred(QCanBusDevice::CanBusError error);

private:
    QCanBusDevice *m_canDevice;
    QString m_deviceId;
};

#endif // CANCOMMUNICATION_H
