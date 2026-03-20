#include "camerafactory.h"
#include "cmvcamera.h"

Camera *CameraFactory::createCamera(CameraType type)
{
    switch (type) {
        case CameraType::MVCodeCamera:
            return new CMvCamera();
        default:
            return nullptr;
    }
}
