#ifndef COMMUNICATIONPARAM_H
#define COMMUNICATIONPARAM_H
/*
 * 定义串口，CAN，UDP接口参数结构体
*/
#include <QString>

// 通信类型枚举
enum class CommunicationType {
    Serial,  // 串口
    Can,     // CAN 总线
    Udp      // UDP 网络
};

// 串口参数结构体
struct SerialParam {
    QString portName;  // 端口名，如 "COM1" 或 "/dev/ttyUSB0"
    qint32 baudRate;   // 波特率
    int dataBits;      // 数据位
    int parity;        // 校验位
    int stopBits;      // 停止位
};

// CAN 总线参数结构体
struct CanParam {
    QString interfaceName;  // 接口名，如 "can0"
    QString pluginName;     // 插件名，如 "socketcan"
};

// UDP 网络参数结构体
struct UdpParam {
    QString ipAddress;  // IP 地址
    quint16 port;       // 端口号
};

// 通信参数结构体
struct CommunicationParam {
    // 标记当前参数对应的通信类型,用于判断该访问哪个参数
    CommunicationType type;
    // 通过 type 判断通信类型的参数
    SerialParam serialParam;
    CanParam canParam;
    UdpParam udpParam;

    // 默认构造（无参初始化，避免错误
    CommunicationParam() : type(CommunicationType::Serial) {}

    // 便捷构造（快速创建对应类型的参数）
    static CommunicationParam serial(const SerialParam& param) {
        CommunicationParam cp;
        cp.type = CommunicationType::Serial;
        cp.serialParam = param;
        return cp;
    }
    static CommunicationParam can(const CanParam& param) {
        CommunicationParam cp;
        cp.type = CommunicationType::Can;
        cp.canParam = param;
        return cp;
    }

    static CommunicationParam udp(const UdpParam& param) {
        CommunicationParam cp;
        cp.type = CommunicationType::Udp;
        cp.udpParam = param;
        return cp;
    }
};

// 设备信息结构体
struct DeviceInfo {
    QString deviceId;                // 设备唯一 ID
    CommunicationType communicationType;  // 通信类型
    CommunicationParam communicationParam;  // 通信参数
};

#endif // COMMUNICATIONPARAM_H

