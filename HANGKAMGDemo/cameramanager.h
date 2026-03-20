#ifndef CAMERAMANAGER_H
#define CAMERAMANAGER_H
#include "camera.h"
#include "CameraParam.h"
#include <QObject>
#include <QMap>
#include <QThread>

// 设备信息结构体，包含通信对象和线程
struct Device {
    Camera *camera;
    QThread *thread;
    DeviceInfo deviceInfo;
};

//相机设备管理器
class Cameramanager: public QObject
{
    Q_OBJECT
public:
    explicit Cameramanager(QObject *parent = nullptr);
    ~Cameramanager();
    // 添加设备
    bool addDevice(const DeviceInfo &deviceInfo);
    // 移除设备
    void removeDevice(const QString &deviceId);
    // 连接设备
    bool connectDevice(const QString &deviceId);
    // 断开设备连接
    void disconnectDevice(const QString &deviceId);
    // 开始抓图
    bool StartGrabbing(const QString &deviceId);
    // 停止抓图
    bool StopGrabbing(const QString &deviceId);
    // 获取设备连接状态
    bool isDeviceConnected(const QString &deviceId) const;
    // 获取所有设备 ID
    QStringList allDeviceIds() const;
signals:
    // 数据接收信号
    void dataReceived(const QString &deviceId, const CameraData &data);
    // 设备连接状态变化信号
    void deviceConnectionStateChanged(const QString &deviceId, bool isConnected);
    // 设备错误信号
    void deviceErrorOccurred(const QString &deviceId, const QString &errorString);

private slots:
    // 转发通信对象的数据接收信号
    void onDataReceived(const QString &deviceId, const CameraData &data);
    // 转发通信对象的连接状态变化信号
    void onConnectionStateChanged(const QString &deviceId, bool isConnected);
    // 转发通信对象的错误信号
    void onErrorOccurred(const QString &deviceId, const QString &errorString);
private:
    // 设备映射表，key 为设备 ID
    QMap<QString, Device> m_devices;
};

#endif // CAMERAMANAGER_H
