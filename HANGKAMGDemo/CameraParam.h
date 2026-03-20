#ifndef CAMERAPARAM_H
#define CAMERAPARAM_H

#include <QString>
#include <QDebug>
#include "Include/MvCodeReaderParams.h"
#define LOGDEBUG qDebug()<<__FILE__<<__LINE__
//相机类型
enum CameraType
{
    MVCodeCamera = 0,   //海康读码相机
};

//相机连接参数--基类
class CameraParam
{
public:
    void *ptr = nullptr;
};

//相机回调数据传输--基类
class CameraData
{
public:
    virtual ~CameraData() = default;
    void *ptr = nullptr;
};

//海康读码相机回调数据--派生类
class MVCodeCameraData : public CameraData
{
public:
    unsigned char *pData;                           //图像信息
    MV_CODEREADER_IMAGE_OUT_INFO_EX2 *pstFrameInfo; //条码信息
};

// 设备信息结构体
struct DeviceInfo {
    QString deviceId;                // 设备唯一 ID
    CameraType cameraType;           // 通信类型
    CameraParam cameraParam;         // 相机连接参数
};

#endif // CAMERAPARAM_H
