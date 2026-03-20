#ifndef CAMERA_H
#define CAMERA_H
#include "CameraParam.h"
#include <QObject>
//相机基类
class Camera : public QObject
{
    Q_OBJECT

public:
    explicit Camera(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~Camera() {}
    // ch:打开设备 | en:Open Device
    virtual int Open(void *param = nullptr) = 0;
    // ch:关闭设备 | en:Close Device
    virtual int Close() = 0;
    // ch:开启抓图 | en:Start Grabbing
    virtual int StartGrabbing() {return true;};
    // ch:停止抓图 | en:Stop Grabbing
    virtual int StopGrabbing() {return true;};
    // 获取设备连接状态
    virtual bool isConnected() const = 0;
    // 获取设备 ID
    virtual QString deviceId() const = 0;
    // 设置设备 ID
    virtual void setDeviceId(const QString &deviceId) = 0;
signals:
    // 数据接收信号
    void dataReceived(const QString &deviceId, const CameraData &data);
    // 连接状态变化信号
    void connectionStateChanged(const QString &deviceId, bool isConnected);
    // 错误信号
    void errorOccurred(const QString &deviceId, const QString &errorString);
};

#endif // CAMERA_H

