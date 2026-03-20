#include "cameramanager.h"
#include "camerafactory.h"
#include <QDebug>
Cameramanager::Cameramanager(QObject *parent)
    :   QObject(parent) {}

Cameramanager::~Cameramanager()
{
    // 移除所有设备
    QStringList deviceIds = m_devices.keys();
    for (const QString &deviceId : deviceIds) {
        removeDevice(deviceId);
    }
}

bool Cameramanager::addDevice(const DeviceInfo &deviceInfo)
{
    if (m_devices.contains(deviceInfo.deviceId)) {
        qWarning() << "设备已经存在:" << deviceInfo.deviceId;
        return false;
    }

    // 创建相机对象
    Camera *camera = CameraFactory::createCamera(deviceInfo.cameraType);
    if (!camera) {
        qWarning() << "无法为设备创建通信:" << deviceInfo.deviceId;
        return false;
    }

    // 设置设备 ID
    camera->setDeviceId(deviceInfo.deviceId);

    // 创建子线程
    QThread *thread = new QThread();
    camera->moveToThread(thread);

    // 连接信号和槽
    connect(camera, &Camera::dataReceived, this, &Cameramanager::onDataReceived);
    connect(camera, &Camera::connectionStateChanged, this, &Cameramanager::onConnectionStateChanged);
    connect(camera, &Camera::errorOccurred, this, &Cameramanager::onErrorOccurred);

    // 启动线程
    thread->start();

    // 保存设备信息
    Device device;
    device.camera = camera;
    device.thread = thread;
    device.deviceInfo = deviceInfo;
    m_devices.insert(deviceInfo.deviceId, device);

    return true;
}

void Cameramanager::removeDevice(const QString &deviceId)
{
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
    delete device.camera;
}

bool Cameramanager::connectDevice(const QString &deviceId)
{
    if (!m_devices.contains(deviceId)) {
        qWarning() << "没有发现设备:" << deviceId;
        return false;
    }

    Device device = m_devices.value(deviceId);
    return device.camera->Open(&device.deviceInfo.cameraParam);
}

void Cameramanager::disconnectDevice(const QString &deviceId)
{
    if (!m_devices.contains(deviceId)) {
        qWarning() << "没有发现设备:" << deviceId;
        return;
    }

    Device device = m_devices.value(deviceId);
    device.camera->Close();
}

bool Cameramanager::StartGrabbing(const QString &deviceId)
{
    if (!m_devices.contains(deviceId)) {
        qWarning() << "没有发现设备:" << deviceId;
        return false;
    }

    Device device = m_devices.value(deviceId);
    return device.camera->StartGrabbing();
}

bool Cameramanager::StopGrabbing(const QString &deviceId)
{
    if (!m_devices.contains(deviceId)) {
        qWarning() << "没有发现设备:" << deviceId;
        return false;
    }

    Device device = m_devices.value(deviceId);
    return device.camera->StopGrabbing();
}

bool Cameramanager::isDeviceConnected(const QString &deviceId) const
{
    if (!m_devices.contains(deviceId)) {
        qWarning() << "没有发现设备:" << deviceId;
        return false;
    }

    const Device &device = m_devices.value(deviceId);
    return device.camera->isConnected();
}

QStringList Cameramanager::allDeviceIds() const
{
    return m_devices.keys();
}

void Cameramanager::onDataReceived(const QString &deviceId, const CameraData &data)
{
    emit dataReceived(deviceId, data);
}

void Cameramanager::onConnectionStateChanged(const QString &deviceId, bool isConnected)
{
    emit deviceConnectionStateChanged(deviceId, isConnected);
}

void Cameramanager::onErrorOccurred(const QString &deviceId, const QString &errorString)
{
    emit deviceErrorOccurred(deviceId, errorString);
}
