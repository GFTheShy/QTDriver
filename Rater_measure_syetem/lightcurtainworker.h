#ifndef LIGHTCURTAINWORKER_H
#define LIGHTCURTAINWORKER_H

#include <QObject>
#include <QTcpSocket>
#include <QVector>
// 匹配底层硬件定义,最大灯珠数量
#define LED_MAXTOTAL 512

//光栅灯珠状态
struct LedNode {
    bool isBlocked = false;//是否遮光
    bool isShielded = false;//是否屏蔽
};

class LightCurtainWorker : public QObject {
    Q_OBJECT
public:
    explicit LightCurtainWorker(const QString &ip, uint16_t port, unsigned int LED_TOTAL,QObject *parent = nullptr);

public slots:
    void connectToDevice();
    void sendRawCommand(const QByteArray &data); // 发送接口
    void SetRasterParameter(const QVector<LedNode> &Leds);      //设置光栅参数
    void GetRasterParameter();      //获取光栅参数
private slots:
    void processPendingData();

signals:
    void dataParsed(const QByteArray &packet);//灯珠状态数据 功能码A1的数据包
    void ShieldedParsed(const QByteArray &packet);//屏蔽的灯珠数据 功能码A7的数据包
    void connectionStatusChanged(const QString &ip, bool connected);

private:
    QString m_ip;
    uint16_t m_port;
    QTcpSocket *m_socket = nullptr;
    QByteArray m_buffer;
    unsigned int FRAME_HEAD = 0x01;//帧头
    unsigned int m_LED_TOTAL = 448;//灯珠数量
    void parsePacket(const QByteArray &frame);
    void sendCommand(uint8_t func, const QByteArray &payload);//发送数据给服务器
};

#endif
