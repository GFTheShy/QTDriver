#ifndef SERIALCOMMUNICATION_H
#define SERIALCOMMUNICATION_H
/*
 * 串口通讯策略类
 * 实现串口的功能底层逻辑
*/
#include <QObject>

#include "ICommunication.h"
#include <QSerialPort>

class SerialCommunication : public ICommunication {
    Q_OBJECT

public:
    explicit SerialCommunication(QObject *parent = nullptr);
    ~SerialCommunication() override;

    // 设备串口连接
    bool connectDevice(const CommunicationParam &param) override;
    // 断开设备连接
    void disconnectDevice() override;
    // 发送串口数据
    bool sendData(const QByteArray &data) override;
    // 判断是否打开串口
    bool isConnected() const override;
    // 获取设备ID
    QString deviceId() const override;
    // 设置设备ID
    void setDeviceId(const QString &deviceId) override;

private slots:
    // 数据接收
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);

private:
    QSerialPort *m_serialPort;
    QString m_deviceId;
};

#endif // SERIALCOMMUNICATION_H

