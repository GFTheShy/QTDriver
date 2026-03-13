#include "communicationmanager.h"

#include "CommunicationFactory.h"
#include <QDebug>

CommunicationManager::CommunicationManager(QObject *parent)
    : QObject(parent) {}

CommunicationManager::~CommunicationManager() {
    // 移除所有设备
    QStringList deviceIds = m_devices.keys();
    for (const QString &deviceId : deviceIds) {
        removeDevice(deviceId);
    }
}

bool CommunicationManager::addDevice(const DeviceInfo &deviceInfo) {
    if (m_devices.contains(deviceInfo.deviceId)) {
        qWarning() << "设备已经存在:" << deviceInfo.deviceId;
        return false;
    }

    // 创建通信对象
    ICommunication *communication = CommunicationFactory::createCommunication(deviceInfo.communicationType);
    if (!communication) {
        qWarning() << "无法为设备创建通信:" << deviceInfo.deviceId;
        return false;
    }

    // 设置设备 ID
    communication->setDeviceId(deviceInfo.deviceId);

    // 创建子线程
    QThread *thread = new QThread();
    communication->moveToThread(thread);

    // 连接信号和槽
    connect(communication, &ICommunication::dataReceived, this, &CommunicationManager::onDataReceived);
    connect(communication, &ICommunication::connectionStateChanged, this, &CommunicationManager::onConnectionStateChanged);
    connect(communication, &ICommunication::errorOccurred, this, &CommunicationManager::onErrorOccurred);

    // 启动线程
    thread->start();

    // 保存设备信息
    Device device;
    device.communication = communication;
    device.thread = thread;
    device.deviceInfo = deviceInfo;
    m_devices.insert(deviceInfo.deviceId, device);

    return true;
}

void CommunicationManager::removeDevice(const QString &deviceId) {
    if (!m_devices.contains(deviceId)) {
        qWarning() << "没有发现设备:" << deviceId;
        return;
    }

    Device device = m_devices.take(deviceId);

    // 断开设备连接
    disconnectDevice(deviceId);

    // 停止并删除线程
    device.thread->quit();
    device.thread->wait();
    delete device.thread;

    // 删除通信对象
    delete device.communication;
}

bool CommunicationManager::connectDevice(const QString &deviceId) {
    if (!m_devices.contains(deviceId)) {
        qWarning() << "没有发现设备:" << deviceId;
        return false;
    }

    Device device = m_devices.value(deviceId);
    return device.communication->connectDevice(device.deviceInfo.communicationParam);
}

void CommunicationManager::disconnectDevice(const QString &deviceId) {
    if (!m_devices.contains(deviceId)) {
        qWarning() << "没有发现设备:" << deviceId;
        return;
    }

    Device device = m_devices.value(deviceId);
    device.communication->disconnectDevice();
}

bool CommunicationManager::sendData(const QString &deviceId, const QByteArray &data) {
    if (!m_devices.contains(deviceId)) {
        qWarning() << "没有发现设备:" << deviceId;
        return false;
    }

    Device device = m_devices.value(deviceId);
    return device.communication->sendData(data);
}

bool CommunicationManager::isDeviceConnected(const QString &deviceId) const {
    if (!m_devices.contains(deviceId)) {
        qWarning() << "没有发现设备:" << deviceId;
        return false;
    }

    const Device &device = m_devices.value(deviceId);
    return device.communication->isConnected();
}

QStringList CommunicationManager::allDeviceIds() const {
    return m_devices.keys();
}

void CommunicationManager::onDataReceived(const QString &deviceId, const QByteArray &data) {
    emit dataReceived(deviceId, data);
}

void CommunicationManager::onConnectionStateChanged(const QString &deviceId, bool isConnected) {
    emit deviceConnectionStateChanged(deviceId, isConnected);
}

void CommunicationManager::onErrorOccurred(const QString &deviceId, const QString &errorString) {
    emit deviceErrorOccurred(deviceId, errorString);
}

