#include "lightcurtainworker.h"
#include <QNetworkProxy>
LightCurtainWorker::LightCurtainWorker(const QString &ip, uint16_t port, unsigned int LED_TOTAL,QObject *parent)
    : QObject(parent), m_ip(ip), m_port(port), m_LED_TOTAL(LED_TOTAL){}

void LightCurtainWorker::connectToDevice() {
    m_socket = new QTcpSocket(this);
    connect(m_socket, &QTcpSocket::readyRead, this, &LightCurtainWorker::processPendingData);
    connect(m_socket, &QTcpSocket::connected, [this](){ emit connectionStatusChanged(m_ip, true); });
    connect(m_socket, &QTcpSocket::disconnected, [this](){ emit connectionStatusChanged(m_ip, false); });
    // 确保 QTcpSocket 尝试直接连接到 IP 地址，而不是通过系统或环境变量配置的代理。
    m_socket->setProxy(QNetworkProxy::NoProxy);
    m_socket->connectToHost(m_ip, m_port);
}

void LightCurtainWorker::sendRawCommand(const QByteArray &data) {
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->write(data);
    }
}

void LightCurtainWorker::SetRasterParameter(const QVector<LedNode> &Leds)
{
    const int STATE_BYTES = (m_LED_TOTAL + 7) / 8;
    // 初始化payload为全0（0=不屏蔽，1=屏蔽）
    QByteArray payload(STATE_BYTES, 0x00);

    // 遍历所有灯珠，设置对应屏蔽位
    for (uint i = 0; i < m_LED_TOTAL; ++i) {
        // 被屏蔽的灯珠
        if (Leds[i].isShielded) {
            int byteIndex = i / 8;    // 灯珠对应的数据字节索引
            int bitPos = i % 8;       // 字节内的位索引（0~7，需与硬件协议位序一致）

            // 彻底解决QByteRef和类型转换问题：取原始指针操作
            unsigned char* dataPtr = reinterpret_cast<unsigned char*>(payload.data());
            dataPtr[byteIndex] |= (1 << bitPos); // 设置对应位为1（屏蔽状态）
        }
    }

    // 直接使用项目现有sendCommand函数发送0xA3功能码指令
    sendCommand(0xA3, payload);

    qDebug() << "已批量设置遮光状态灯珠为屏蔽状态，数据字节数：" << STATE_BYTES;
}

void LightCurtainWorker::GetRasterParameter()
{
    QByteArray readPayload;
    //获取灯珠的屏蔽状态
    sendCommand(0xA7, QByteArray()); // [cite: 3]
}

void LightCurtainWorker::processPendingData() {
    QByteArray data = m_socket->readAll();
    //qDebug() << "收到原始字节数：" << data.size() << "内容：" << data.toHex(' ');
    // 1. 将新到的数据追加到缓冲区
    m_buffer.append(data);
    //qDebug()<<"读取数据"<<m_buffer;
    // 2. 循环处理缓冲区，直到没有完整的帧为止
    // 最小帧长度通常为：帧头(1) + 功能码(1) + 长度(2) = 4字节
    while (m_buffer.size() >= 4) {
        // 检查帧头（默认协议地址码/帧头为 0x01）
        if (static_cast<uint8_t>(m_buffer[0]) != FRAME_HEAD) {
            m_buffer.remove(0, 1); // 如果不是帧头，丢弃该字节继续寻找
            continue;
        }

        // 解析长度字段（高字节在索引2，低字节在索引3）
        uint16_t frameLen = (static_cast<uint8_t>(m_buffer[2]) << 8) | static_cast<uint8_t>(m_buffer[3]);

        // 3. 检查当前缓冲区的数据是否足够一整帧
        if (m_buffer.size() < frameLen) {
            // 数据还没收全（半包），跳出循环，等待下一次 readyRead
            break;
        }

        // 4. 提取出完整的一帧
        QByteArray completeFrame = m_buffer.left(frameLen);

        // 5. 调用原来的解析函数
        parsePacket(completeFrame);

        // 6. 从缓冲区中删除已经处理掉的这一帧
        m_buffer.remove(0, frameLen);
    }
}

void LightCurtainWorker::parsePacket(const QByteArray &frame) {
    if(frame.isEmpty()) return;
    uint8_t func = (uint8_t)frame[1];
    switch (func)
    {
        case 0xA1:
            {
                emit dataParsed(frame);
            }
            break;
        case 0xA7: // 灯珠的屏蔽状态 [cite: 1]
            {
                emit ShieldedParsed(frame);
            }
           break;
        default:
            qDebug()<<"未知响应";
            break;
    }

}

void LightCurtainWorker::sendCommand(uint8_t func, const QByteArray &payload)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState) return;
    QByteArray pkg;
    pkg.append((char)FRAME_HEAD);
    pkg.append((char)func);
    uint16_t len = payload.size() + 4;
    pkg.append((char)(len >> 8));
    pkg.append((char)(len & 0xFF));
    pkg.append(payload);
    m_socket->write(pkg);
    qDebug()<<"发送数据："<<pkg;
    m_socket->flush(); // 强制发送
}
