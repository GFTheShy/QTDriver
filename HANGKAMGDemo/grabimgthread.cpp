#include "grabimgthread.h"

GrabImgThread::GrabImgThread(int cameraId)
    : m_cameraId(cameraId)
{

}

GrabImgThread::~GrabImgThread()
{

}

//设置相机指针
void GrabImgThread::setCameraPtr(CMvCamera *camera)
{
    m_cameraPtr = camera;

    //注册异常回调函数
    //TODO:


    //注册回调函数
    int nRet = camera->RegisterImageCallBack(0,ImageCallback,this);
    if(MV_CODEREADER_OK != nRet)
    {
        LOGDEBUG<<"m_cameraId:"<<m_cameraId<<"注册回调函数失败！";
        return;
    }
}

//线程运行
void GrabImgThread::run()
{

}

//回调函数
void __stdcall GrabImgThread::ImageCallback(unsigned char * pData, MV_CODEREADER_IMAGE_OUT_INFO_EX2* pstFrameInfo, void* pUser)
{
    LOGDEBUG<<QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss:zzz ")<<"回调函数执行了";
    GrabImgThread* pThread = static_cast<GrabImgThread *>(pUser);

    //创建QImage对象
    QImage showImage = QImage(pData,pstFrameInfo->nWidth,pstFrameInfo->nHeight,QImage::Format_RGB888);

    //发送信号,通知主界面更新图像
    emit pThread->signal_imageReady(showImage,pstFrameInfo,pThread->m_cameraId);
}

