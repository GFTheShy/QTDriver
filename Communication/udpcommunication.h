#ifndef UDPCOMMUNICATION_H
#define UDPCOMMUNICATION_H


#include <QObject>

#include "ICommunication.h"
#include <QUdpSocket>

class UdpCommunication : public ICommunication {
    Q_OBJECT

public:
    explicit UdpCommunication(QObject *parent = nullptr);
    ~UdpCommunication() override;
    // 连接UDP设备
    bool connectDevice(const CommunicationParam &param) override;
    // 断开连接
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
    void onReadyRead();
    void onErrorOccurred(int error);

private:
    QUdpSocket *m_udpSocket;
    QString m_deviceId;
    QString m_ipAddress;
    quint16 m_port;
};
#endif // UDPCOMMUNICATION_H
