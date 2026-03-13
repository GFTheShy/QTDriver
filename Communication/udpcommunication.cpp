#include "udpcommunication.h"

#include <QDebug>

UdpCommunication::UdpCommunication(QObject *parent)
    : ICommunication(parent),
      m_udpSocket(new QUdpSocket(this)),
      m_port(0) {
    connect(m_udpSocket, &QUdpSocket::readyRead, this, &UdpCommunication::onReadyRead);
    connect(m_udpSocket, QOverload<QAbstractSocket::SocketError>::of(&QUdpSocket::error), this, &UdpCommunication::onErrorOccurred);
}

UdpCommunication::~UdpCommunication() {
    disconnectDevice();
}

bool UdpCommunication::connectDevice(const CommunicationParam &param) {
    if (isConnected()) {
        disconnectDevice();
    }

    const UdpParam &udpParam = param.udpParam;
    m_ipAddress = udpParam.ipAddress;
    m_port = udpParam.port;

    // UDP 是无连接的，这里只是记录目标地址和端口
    emit connectionStateChanged(m_deviceId, true);
    return true;
}

void UdpCommunication::disconnectDevice() {
    // UDP 没有实际的连接，只需重置地址和端口
    m_ipAddress.clear();
    m_port = 0;
    emit connectionStateChanged(m_deviceId, false);
}

bool UdpCommunication::sendData(const QByteArray &data) {
    if (m_ipAddress.isEmpty() || m_port == 0) {
        emit errorOccurred(m_deviceId, "UDP地址或端口没设置");
        return false;
    }

    qint64 bytesWritten = m_udpSocket->writeDatagram(data, QHostAddress(m_ipAddress), m_port);
    if (bytesWritten != data.size()) {
        emit errorOccurred(m_deviceId, "未能发送所有数据");
        return false;
    }

    return true;
}

bool UdpCommunication::isConnected() const {
    return !m_ipAddress.isEmpty() && m_port != 0;
}

QString UdpCommunication::deviceId() const {
    return m_deviceId;
}

void UdpCommunication::setDeviceId(const QString &deviceId) {
    m_deviceId = deviceId;
}

void UdpCommunication::onReadyRead() {
    while (m_udpSocket->hasPendingDatagrams()) {
        QByteArray data;
        QHostAddress senderAddress;
        quint16 senderPort;

        data.resize(m_udpSocket->pendingDatagramSize());
        m_udpSocket->readDatagram(data.data(), data.size(), &senderAddress, &senderPort);

        // 只接收来自目标地址的数据
        if (senderAddress.toString() == m_ipAddress && senderPort == m_port) {
            emit dataReceived(m_deviceId, data);
        }
    }
}

void UdpCommunication::onErrorOccurred(int error)
{
    QAbstractSocket::SocketError err = static_cast<QAbstractSocket::SocketError>(error);
    if (error != 0)
    {
        qDebug() << QString("UDP错误：") << m_udpSocket->errorString();
        qDebug() << QString("错误码：") << err;
        emit errorOccurred(m_deviceId, m_udpSocket->errorString());
    }
}

