#include "cancommunication.h"
#include <QDebug>

CanCommunication::CanCommunication(QObject *parent)
    : ICommunication(parent),
      m_canDevice(nullptr) {}

CanCommunication::~CanCommunication() {
    disconnectDevice();
}

bool CanCommunication::connectDevice(const CommunicationParam &param) {
    if (isConnected()) {
        disconnectDevice();
    }

    const CanParam &canParam = param.canParam;
    QCanBus *canBus = QCanBus::instance();
    if (!canBus) {
        emit errorOccurred(m_deviceId, "创建QCanBus实例失败");
        return false;
    }

    //没有availablePlugins函数
    //QStringList availablePlugins = canBus->availablePlugins();
    QStringList availablePlugins = canBus->plugins();
    if (!availablePlugins.contains(canParam.pluginName)) {
        emit errorOccurred(m_deviceId, "CAN插件不可用:" + canParam.pluginName);
        return false;
    }

    m_canDevice = canBus->createDevice(canParam.pluginName, canParam.interfaceName);
    if (!m_canDevice) {
        emit errorOccurred(m_deviceId, "无法创建CAN设备");
        return false;
    }

    connect(m_canDevice, &QCanBusDevice::framesReceived, this, &CanCommunication::onFramesReceived);
    connect(m_canDevice, &QCanBusDevice::errorOccurred, this, &CanCommunication::onErrorOccurred);

    bool success = m_canDevice->connectDevice();
    emit connectionStateChanged(m_deviceId, success);
    if (!success) {
        emit errorOccurred(m_deviceId, m_canDevice->errorString());
        delete m_canDevice;
        m_canDevice = nullptr;
    }
    return success;
}

void CanCommunication::disconnectDevice() {
    if (m_canDevice && m_canDevice->state() == QCanBusDevice::ConnectedState)
    {
        m_canDevice->disconnectDevice();
        emit connectionStateChanged(m_deviceId, false);
        delete m_canDevice;
        m_canDevice = nullptr;
    }
}

bool CanCommunication::sendData(const QByteArray &data) {
    if (!isConnected()) {
        emit errorOccurred(m_deviceId, "设备没有连接");
        return false;
    }

    if (data.size() > 8) {
        emit errorOccurred(m_deviceId, "CAN数据长度超过8字节");
        return false;
    }

    QCanBusFrame frame;
    frame.setPayload(data);
    bool success = m_canDevice->writeFrame(frame);
    if (!success) {
        emit errorOccurred(m_deviceId, "发送CAN帧失败");
    }
    return success;
}

bool CanCommunication::isConnected() const {
    return m_canDevice && (m_canDevice->state() == QCanBusDevice::ConnectedState);
}

QString CanCommunication::deviceId() const {
    return m_deviceId;
}

void CanCommunication::setDeviceId(const QString &deviceId) {
    m_deviceId = deviceId;
}

void CanCommunication::onFramesReceived() {
    if (!m_canDevice) return;

    while (m_canDevice->framesAvailable()) {
        QCanBusFrame frame = m_canDevice->readFrame();
        emit dataReceived(m_deviceId, frame.payload());
    }
}

void CanCommunication::onErrorOccurred(QCanBusDevice::CanBusError error) {
    if (error != QCanBusDevice::NoError) {
        emit errorOccurred(m_deviceId, m_canDevice->errorString());
    }
}

