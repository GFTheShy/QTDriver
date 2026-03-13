#include <QCoreApplication>
#include <QDebug>
#include <QTimer>
#include "communicationmanager.h"
#include "CommunicationParam.h"

int main(int argc, char *argv[]) {
    QCoreApplication a(argc, argv);

    // 创建通信管理器
    CommunicationManager manager;

    // 连接信号和槽，用于接收设备状态和数据
    QObject::connect(&manager, &CommunicationManager::dataReceived,
                     [](const QString &deviceId, const QByteArray &data) {
        qDebug() << "接收到的数据，从设备 " << deviceId << ":" << data.toHex();
    });

    QObject::connect(&manager, &CommunicationManager::deviceConnectionStateChanged,
                     [](const QString &deviceId, bool isConnected) {
        qDebug() << "设备" << deviceId << "连接状态:" << (isConnected ? "Connected" : "Disconnected");
    });

    QObject::connect(&manager, &CommunicationManager::deviceErrorOccurred,
                     [](const QString &deviceId, const QString &errorString) {
        qDebug() << "设备" << deviceId << "错误:" << errorString;
    });

    // 添加串口设备 1
    DeviceInfo serialDevice1;
    serialDevice1.deviceId = "SerialDevice1";
    serialDevice1.communicationType = CommunicationType::Serial;
    serialDevice1.communicationParam.serialParam.portName = "COM1";
    serialDevice1.communicationParam.serialParam.baudRate = 9600;
    serialDevice1.communicationParam.serialParam.dataBits = 8;
    serialDevice1.communicationParam.serialParam.parity = 0;
    serialDevice1.communicationParam.serialParam.stopBits = 1;
    manager.addDevice(serialDevice1);

    // 添加串口设备 2
    DeviceInfo serialDevice2;
    serialDevice2.deviceId = "SerialDevice2";
    serialDevice2.communicationType = CommunicationType::Serial;
    serialDevice2.communicationParam.serialParam.portName = "COM2";
    serialDevice2.communicationParam.serialParam.baudRate = 115200;
    serialDevice2.communicationParam.serialParam.dataBits = 8;
    serialDevice2.communicationParam.serialParam.parity = 0;
    serialDevice2.communicationParam.serialParam.stopBits = 1;
    manager.addDevice(serialDevice2);

    // 添加 CAN 设备 1
    DeviceInfo canDevice1;
    canDevice1.deviceId = "CanDevice1";
    canDevice1.communicationType = CommunicationType::Can;
    canDevice1.communicationParam.canParam.interfaceName = "can0";
    canDevice1.communicationParam.canParam.pluginName = "socketcan";
    manager.addDevice(canDevice1);

    // 添加 CAN 设备 2
    DeviceInfo canDevice2;
    canDevice2.deviceId = "CanDevice2";
    canDevice2.communicationType = CommunicationType::Can;
    canDevice2.communicationParam.canParam.interfaceName = "can1";
    canDevice2.communicationParam.canParam.pluginName = "socketcan";
    manager.addDevice(canDevice2);

    // 添加 UDP 设备 1
    DeviceInfo udpDevice1;
    udpDevice1.deviceId = "UdpDevice1";
    udpDevice1.communicationType = CommunicationType::Udp;
    udpDevice1.communicationParam.udpParam.ipAddress = "192.168.1.100";
    udpDevice1.communicationParam.udpParam.port = 8080;
    manager.addDevice(udpDevice1);

    // 添加 UDP 设备 2
    DeviceInfo udpDevice2;
    udpDevice2.deviceId = "UdpDevice2";
    udpDevice2.communicationType = CommunicationType::Udp;
    udpDevice2.communicationParam.udpParam.ipAddress = "192.168.1.101";
    udpDevice2.communicationParam.udpParam.port = 8081;
    manager.addDevice(udpDevice2);

    // 连接所有设备
    QStringList deviceIds = manager.allDeviceIds();
    for (const QString &deviceId : deviceIds) {
        manager.connectDevice(deviceId);
    }

    // 向每个设备发送数据
    for (const QString &deviceId : deviceIds) {
        QByteArray data = "Hello, " + deviceId.toUtf8();
        manager.sendData(deviceId, data);
    }

    // 等待一段时间（10s）后断开所有设备连接
    QTimer::singleShot(10000, [&manager]() {
        QStringList deviceIds = manager.allDeviceIds();
        for (const QString &deviceId : deviceIds) {
            manager.disconnectDevice(deviceId);
        }
    });

    // 等待一段时间（15s）后移除所有设备
    QTimer::singleShot(15000, [&manager, &a]() {
        QStringList deviceIds = manager.allDeviceIds();
        for (const QString &deviceId : deviceIds) {
            manager.removeDevice(deviceId);
        }
        a.quit();
    });

    return a.exec();
}
