#include "SerialCommunication.h"

#include <QDebug>

SerialCommunication::SerialCommunication(QObject *parent)
    : ICommunication(parent),
      m_serialPort(new QSerialPort(this)) {
    connect(m_serialPort, &QSerialPort::readyRead, this, &SerialCommunication::onReadyRead);
    connect(m_serialPort, &QSerialPort::errorOccurred, this, &SerialCommunication::onErrorOccurred);
}

SerialCommunication::~SerialCommunication() {
    disconnectDevice();
}

bool SerialCommunication::connectDevice(const CommunicationParam &param) {
    if (isConnected()) {
        disconnectDevice();
    }

    const SerialParam &serialParam = param.serialParam;
    m_serialPort->setPortName(serialParam.portName);
    m_serialPort->setBaudRate(serialParam.baudRate);
    m_serialPort->setDataBits(static_cast<QSerialPort::DataBits>(serialParam.dataBits));
    m_serialPort->setParity(static_cast<QSerialPort::Parity>(serialParam.parity));
    m_serialPort->setStopBits(static_cast<QSerialPort::StopBits>(serialParam.stopBits));
    m_serialPort->setFlowControl(QSerialPort::NoFlowControl);

    bool success = m_serialPort->open(QIODevice::ReadWrite);
    emit connectionStateChanged(m_deviceId, success);
    if (!success) {
        emit errorOccurred(m_deviceId, m_serialPort->errorString());
    }
    return success;
}

void SerialCommunication::disconnectDevice() {
    if (m_serialPort->isOpen()) {
        m_serialPort->close();
        emit connectionStateChanged(m_deviceId, false);
    }
}

bool SerialCommunication::sendData(const QByteArray &data) {
    if (!isConnected()) {
        emit errorOccurred(m_deviceId, "没有连接设备");
        return false;
    }

    qint64 bytesWritten = m_serialPort->write(data);
    if (bytesWritten != data.size()) {
        emit errorOccurred(m_deviceId, "未能发送所有数据");
        return false;
    }

    return true;
}

bool SerialCommunication::isConnected() const {
    return m_serialPort->isOpen();
}

QString SerialCommunication::deviceId() const {
    return m_deviceId;
}

void SerialCommunication::setDeviceId(const QString &deviceId) {
    m_deviceId = deviceId;
}

void SerialCommunication::onReadyRead() {
    QByteArray data = m_serialPort->readAll();
    emit dataReceived(m_deviceId, data);
}

void SerialCommunication::onErrorOccurred(QSerialPort::SerialPortError error) {
    if (error != QSerialPort::NoError) {
        emit errorOccurred(m_deviceId, m_serialPort->errorString());
    }
}

