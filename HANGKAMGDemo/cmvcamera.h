/************************************************************************/
/* 以C++接口为基础，对常用函数进行二次封装，方便用户使用                */
/************************************************************************/

#ifndef _MV_CAMERA_H_
#define _MV_CAMERA_H_

#include <string.h>
#include "Include/MvCodeReaderCtrl.h"
#include <QDebug>

#define LOGDEBUG qDebug()<<__FILE__<<__LINE__

//会跟系统函数定义冲突
//using namespace cv;

#ifndef MV_NULL
#define MV_NULL    0
#endif

class CMvCamera
{
public:
    CMvCamera();
    ~CMvCamera();

    // ch:获取SDK版本号 | en:Get SDK Version
    static int GetSDKVersion();

    // ch:枚举设备 | en:Enumerate Device
    static int EnumDevices(IN OUT MV_CODEREADER_DEVICE_INFO_LIST* pstDevList, IN unsigned int nTLayerType = MV_CODEREADER_GIGE_DEVICE);

    // ch:判断设备是否可达 | en:Is the device accessible
    static bool IsDeviceAccessible(IN MV_CODEREADER_DEVICE_INFO* pstDevInfo, IN unsigned int nAccessMode);

    // ch:打开设备 | en:Open Device
    int Open(IN const MV_CODEREADER_DEVICE_INFO* pstDevInfo);

    // ch:关闭设备 | en:Close Device
    int Close();

    // ch:注册图像数据回调 | en:Register Image Data CallBack
    int RegisterImageCallBack(unsigned int nChannelID,
                              void(__stdcall* cbOutput)(unsigned char * pData, MV_CODEREADER_IMAGE_OUT_INFO_EX2* pstFrameInfo, void* pUser),
                              void* pUser);

    // ch:开启抓图 | en:Start Grabbing
    int StartGrabbing();

    // ch:停止抓图 | en:Stop Grabbing
    int StopGrabbing();


    // ch:获取设备信息 | en:Get device information
    int GetDeviceInfo(IN MV_CODEREADER_DEVICE_INFO* pstDevInfo);

    // ch:获取和设置Int型参数，如 Width和Height，详细内容参考SDK安装目录下的 MvCameraNode.xlsx 文件
    // en:Get Int type parameters, such as Width and Height, for details please refer to MvCameraNode.xlsx file under SDK installation directory
    int GetIntValue(IN const char* strKey,IN OUT MV_CODEREADER_INTVALUE_EX *pIntValue);
    int SetIntValue(IN const char* strKey, IN int64_t nValue);

    // ch:获取和设置Enum型参数，如 PixelFormat，详细内容参考SDK安装目录下的 MvCameraNode.xlsx 文件
    // en:Get Enum type parameters, such as PixelFormat, for details please refer to MvCameraNode.xlsx file under SDK installation directory
    int GetEnumValue(IN const char* strKey, IN OUT MV_CODEREADER_ENUMVALUE *pEnumValue);
    int SetEnumValue(IN const char* strKey, IN unsigned int nValue);
    int SetEnumValueByString(IN const char* strKey,IN const char* sValue);

    // ch:获取和设置Float型参数，如 ExposureTime和Gain，详细内容参考SDK安装目录下的 MvCameraNode.xlsx 文件
    // en:Get Float type parameters, such as ExposureTime and Gain, for details please refer to MvCameraNode.xlsx file under SDK installation directory
    int GetFloatValue(IN const char* strKey, IN OUT MV_CODEREADER_FLOATVALUE *pFloatValue);
    int SetFloatValue(IN const char* strKey, IN float fValue);

    // ch:获取和设置Bool型参数，如 ReverseX，详细内容参考SDK安装目录下的 MvCameraNode.xlsx 文件
    // en:Get Bool type parameters, such as ReverseX, for details please refer to MvCameraNode.xlsx file under SDK installation directory
    int GetBoolValue(IN const char* strKey,IN OUT bool *pBoolValue);
    int SetBoolValue(IN const char* strKey, IN bool bValue);

    // ch:获取和设置String型参数，如 DeviceUserID，详细内容参考SDK安装目录下的 MvCameraNode.xlsx 文件UserSetSave
    // en:Get String type parameters, such as DeviceUserID, for details please refer to MvCameraNode.xlsx file under SDK installation directory
    int GetStringValue(IN const char* strKey, IN OUT MV_CODEREADER_STRINGVALUE *pStringValue);
    int SetStringValue(IN const char* strKey, IN const char * strValue);

    // ch:执行一次Command型命令，如 UserSetSave，详细内容参考SDK安装目录下的 MvCameraNode.xlsx 文件
    // en:Execute Command once, such as UserSetSave, for details please refer to MvCameraNode.xlsx file under SDK installation directory
    int CommandExecute(IN const char* strKey);

    // ch:探测网络最佳包大小(只对GigE相机有效) | en:Detection network optimal package size(It only works for the GigE camera)
    int GetOptimalPacketSize(unsigned int* pOptimalPacketSize);

    // ch:注册消息异常回调 | en:Register Message Exception CallBack
    int RegisterExceptionCallBack(void(__stdcall* cbException)(unsigned int nMsgType, void* pUser), void* pUser);

    // ch:强制IP | en:Force IP
    int ForceIp(unsigned int nIP, unsigned int nSubNetMask, unsigned int nDefaultGateWay);

    // ch:配置IP方式 | en:IP configuration method
    int SetIpConfig(unsigned int nType);

    // ch:保存图片 | en:save image
    int SaveImage(IN OUT MV_CODEREADER_SAVE_IMAGE_PARAM_EX* pSaveParam);

private:

    void *m_hDevHandle;
};

#endif//_MV_CAMERA_H_
