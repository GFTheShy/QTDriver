#ifndef CAMERAFACTORY_H
#define CAMERAFACTORY_H
#include "camera.h"
#include "CameraParam.h"

//相机工厂
class CameraFactory
{
public:
    // 创建相机
    static Camera *createCamera(CameraType type);
};

#endif // CAMERAFACTORY_H


