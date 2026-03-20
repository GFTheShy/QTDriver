#include "cmvcamera.h"

CMvCamera::CMvCamera()
{
    m_hDevHandle = MV_NULL;
}

CMvCamera::~CMvCamera()
{
    if (m_hDevHandle)
    {
        MV_CODEREADER_DestroyHandle(m_hDevHandle);
        m_hDevHandle = MV_NULL;
    }
}

int CMvCamera::GetSDKVersion()
{
    return MV_CODEREADER_GetSDKVersion();
}

int CMvCamera::EnumDevices(MV_CODEREADER_DEVICE_INFO_LIST *pstDevList, unsigned int nTLayerType)
{
    return MV_CODEREADER_EnumDevices(pstDevList,nTLayerType);
}

bool CMvCamera::IsDeviceAccessible(MV_CODEREADER_DEVICE_INFO *pstDevInfo, unsigned int nAccessMode)
{
    return MV_CODEREADER_IsDeviceAccessible(pstDevInfo, nAccessMode);
}

int CMvCamera::Open(void *param)
{
    IN const MV_CODEREADER_DEVICE_INFO* pstDevInfo = (IN const MV_CODEREADER_DEVICE_INFO*) param;
    if (MV_NULL == pstDevInfo)
    {
        return MV_CODEREADER_E_PARAMETER;
    }

    if (m_hDevHandle)
    {
        return MV_CODEREADER_E_CALLORDER;
    }

    int nRet  = MV_CODEREADER_CreateHandle(&m_hDevHandle, pstDevInfo);
    if (MV_CODEREADER_OK != nRet)
    {
        emit errorOccurred(m_deviceId, QString("设备ID:%1,创建句柄函数失败.错误码:%2").arg(m_deviceId).arg(nRet));
        return nRet;
    }

    nRet = MV_CODEREADER_OpenDevice(m_hDevHandle);
    if (MV_CODEREADER_OK != nRet)
    {
        MV_CODEREADER_DestroyHandle(m_hDevHandle);
        m_hDevHandle = MV_NULL;
        emit errorOccurred(m_deviceId, QString("设备ID:%1,打开设备函数失败.错误码:%2").arg(m_deviceId).arg(nRet));
        return nRet;
    }
    //注册回调函数
    nRet = RegisterImageCallBack(0,ImageCallback,this);
    if(MV_CODEREADER_OK != nRet)
    {
        LOGDEBUG<<"设备ID:"<<m_deviceId<<"注册回调函数失败！";
        emit errorOccurred(m_deviceId, QString("设备ID:%1,注册回调函数失败.错误码:%2").arg(m_deviceId).arg(nRet));
    }

    emit connectionStateChanged(m_deviceId, true);
    return nRet;
}

int CMvCamera::Close()
{
    if (MV_NULL == m_hDevHandle)
    {
        return MV_CODEREADER_E_HANDLE;
    }

    MV_CODEREADER_CloseDevice(m_hDevHandle);

    int nRet = MV_CODEREADER_DestroyHandle(m_hDevHandle);
    m_hDevHandle = MV_NULL;
    emit connectionStateChanged(m_deviceId, false);
    return nRet;
}

int CMvCamera::RegisterImageCallBack(unsigned int nChannelID, void (__stdcall* cbOutput)(unsigned char *, MV_CODEREADER_IMAGE_OUT_INFO_EX2 *, void *), void *pUser)
{
    return MV_CODEREADER_MSC_RegisterImageCallBack(m_hDevHandle, nChannelID,cbOutput,pUser);
}

int CMvCamera::StartGrabbing()
{
    return MV_CODEREADER_StartGrabbing(m_hDevHandle);
}

int CMvCamera::StopGrabbing()
{
    return MV_CODEREADER_StopGrabbing(m_hDevHandle);
}

int CMvCamera::GetDeviceInfo(MV_CODEREADER_DEVICE_INFO *pstDevInfo)
{
    return MV_CODEREADER_GetDeviceInfo(m_hDevHandle, pstDevInfo);
}

int CMvCamera::GetIntValue(const char *strKey, MV_CODEREADER_INTVALUE_EX *pIntValue)
{
    return MV_CODEREADER_GetIntValue(m_hDevHandle, strKey, pIntValue);
}

int CMvCamera::SetIntValue(const char *strKey, int64_t nValue)
{
    return MV_CODEREADER_SetIntValue(m_hDevHandle, strKey, nValue);
}

int CMvCamera::GetEnumValue(const char *strKey, MV_CODEREADER_ENUMVALUE *pEnumValue)
{
    return MV_CODEREADER_GetEnumValue(m_hDevHandle, strKey, pEnumValue);
}

int CMvCamera::SetEnumValue(const char *strKey, unsigned int nValue)
{
    return MV_CODEREADER_SetEnumValue(m_hDevHandle, strKey, nValue);
}

int CMvCamera::SetEnumValueByString(const char *strKey, const char *sValue)
{
    return MV_CODEREADER_SetEnumValueByString(m_hDevHandle, strKey, sValue);
}

int CMvCamera::GetFloatValue(const char *strKey, MV_CODEREADER_FLOATVALUE *pFloatValue)
{
    return MV_CODEREADER_GetFloatValue(m_hDevHandle, strKey, pFloatValue);
}

int CMvCamera::SetFloatValue(const char *strKey, float fValue)
{
    return MV_CODEREADER_SetFloatValue(m_hDevHandle, strKey, fValue);
}

int CMvCamera::GetBoolValue(const char *strKey, bool *pBoolValue)
{
    return MV_CODEREADER_GetBoolValue(m_hDevHandle, strKey, pBoolValue);
}

int CMvCamera::SetBoolValue(const char *strKey, bool bValue)
{
    return MV_CODEREADER_SetBoolValue(m_hDevHandle, strKey, bValue);
}

int CMvCamera::GetStringValue(const char *strKey, MV_CODEREADER_STRINGVALUE *pStringValue)
{
    return MV_CODEREADER_GetStringValue(m_hDevHandle, strKey, pStringValue);
}

int CMvCamera::SetStringValue(const char *strKey, const char *strValue)
{
    return MV_CODEREADER_SetStringValue(m_hDevHandle, strKey, strValue);
}

int CMvCamera::CommandExecute(const char *strKey)
{
    return MV_CODEREADER_SetCommandValue(m_hDevHandle, strKey);
}

int CMvCamera::GetOptimalPacketSize(unsigned int *pOptimalPacketSize)
{
    if (MV_NULL == pOptimalPacketSize)
    {
        return MV_CODEREADER_E_PARAMETER;
    }

    int nRet = MV_CODEREADER_GetOptimalPacketSize(m_hDevHandle);
    if (nRet < MV_CODEREADER_OK)
    {
        return nRet;
    }

    *pOptimalPacketSize = (unsigned int)nRet;

    return MV_CODEREADER_OK;
}

int CMvCamera::RegisterExceptionCallBack(void(__stdcall* cbException)(unsigned int nMsgType, void* pUser), void* pUser)
{
    return MV_CODEREADER_RegisterExceptionCallBack(m_hDevHandle, cbException, pUser);
}

int CMvCamera::ForceIp(unsigned int nIP, unsigned int nSubNetMask, unsigned int nDefaultGateWay)
{
    return MV_CODEREADER_GIGE_ForceIp(m_hDevHandle, nIP, nSubNetMask, nDefaultGateWay);
}

int CMvCamera::SetIpConfig(unsigned int nType)
{
    return MV_CODEREADER_GIGE_SetIpConfig(m_hDevHandle, nType);
}

int CMvCamera::SaveImage(MV_CODEREADER_SAVE_IMAGE_PARAM_EX *pSaveParam)
{
    return MV_CODEREADER_SaveImage(m_hDevHandle, pSaveParam);
}

void CMvCamera::ImageCallback(unsigned char *pData, MV_CODEREADER_IMAGE_OUT_INFO_EX2 *pstFrameInfo, void *pUser)
{
    LOGDEBUG<<"相机调用了海康的回调函数";
    // 1. 转换pUser为对象指针
    CMvCamera* instance = static_cast<CMvCamera*>(pUser);

    // 发射数据结构体
    MVCodeCameraData CodeData;
    CodeData.pData = pData;
    CodeData.pstFrameInfo = pstFrameInfo;
    emit instance->dataReceived(instance->m_deviceId,CodeData);
}

bool CMvCamera::isConnected() const
{
    if (m_hDevHandle)
    {
        return true;
    }
    return false;
}

QString CMvCamera::deviceId() const
{
    return m_deviceId;
}

void CMvCamera::setDeviceId(const QString &deviceId)
{
    m_deviceId = deviceId;
}
