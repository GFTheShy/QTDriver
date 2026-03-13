#ifndef MAIN_H
#define MAIN_H

#include <QWidget>
#include <QTcpSocket>
#include <QLabel>
#include <QVector>
#include <QTimer>
#include <QElapsedTimer>
#include <QMessageBox>
#include <QDebug>
#include <QNetworkProxy>
#include <QThread>

// 匹配底层硬件定义,最大灯珠数量
#define LED_MAXTOTAL 512

struct LoadMessage{
    // 工作状态枚举
    enum WorkStatus{
        WorkScaning = 0,    // 主动模式，持续扫描工作状态
        WorkSettingShield,  // 应答模式，不扫描等待指令操作
    };
    enum ScanMode{
      RunningScan = 0,   // 全扫扫描模式
      QuickScanMode = 1, // 跳扫扫描模式
    };

    //光栅连接设置
    uint8_t IP_ADDRESS[4] = {0};                //IP地址
    uint8_t NETMASK_ADDRESS[4] = {0};           //子网掩码
    uint8_t GATEWAY_ADDRESS[4] = {0};           //网关地址
    uint8_t FRAME_HEADER = 0x00;                // 帧头/地址码
    uint16_t TCP_ECHO_PORT = 0;                 // TCP 服务器端口
    //光栅基本设置
    uint16_t firmwaveDeviceVersion = 0;         //当前固件版本
    WorkStatus WorkStatusMode = WorkScaning;    //当前工作状态
    ScanMode ScanModeValue = RunningScan;       //当前扫描状态
    uint16_t LED_TOTAL = 0;                     //当前灯珠总数
    uint16_t RasterSpace = 0;                   //当前灯珠间距
    uint16_t step = 0;                          //跳扫模式下的扫描步长，步长可以根据需求调整（8, 16, 32）
    uint16_t OneTime = 0;                       //单个灯珠的检测时间
};
#endif // MAIN_H
