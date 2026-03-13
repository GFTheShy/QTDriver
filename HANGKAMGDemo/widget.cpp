#include "widget.h"
#include "ui_widget.h"
#include <QDir>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    // 初始化保存路径（例如：当前程序目录下的images文件夹）
    m_savePath = QCoreApplication::applicationDirPath() + "/images";
    QDir dir(m_savePath);
    if (!dir.exists()) {
        dir.mkpath("."); // 若文件夹不存在
    }

    this->initWidget();
    connect(ui->pushButton,&QPushButton::clicked,this,&Widget::pb_Init);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::initWidget()
{
    for(int i=0;i<CAMERA_NUM;i++)
    {
        //相机对象
        m_myCamera[i] = new CMvCamera;

        //线程对象
        m_grabThread[i] = new GrabImgThread(i);
        connect(m_grabThread[i],&GrabImgThread::signal_imageReady,this,&Widget::slot_imageReady,Qt::BlockingQueuedConnection);
    }
}

void Widget::slot_imageReady(const QImage &image, MV_CODEREADER_IMAGE_OUT_INFO_EX2* pstFrameInfo ,int cameraId)
{
    QPixmap showPixmap = QPixmap::fromImage(image).scaled(ui->label->size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
    QString barcodes;
    //提取条码出来
    if (pstFrameInfo && pstFrameInfo->UnparsedBcrList.pstCodeListEx2)
    {
        unsigned int nCodeNum = pstFrameInfo->UnparsedBcrList.pstCodeListEx2->nCodeNum;
        MV_CODEREADER_RESULT_BCR_EX2* stBcrResult = (MV_CODEREADER_RESULT_BCR_EX2*)pstFrameInfo->UnparsedBcrList.pstCodeListEx2;
        if(!stBcrResult) return;
        for (unsigned int i = 0; i < stBcrResult->nCodeNum; i++)
        {
            if (i > 0) {
                barcodes += ","; // 非首元素前添加逗号
            }
            barcodes += QString::fromLocal8Bit(stBcrResult->stBcrInfoEx2[i].chCode);
        }
    }
    LOGDEBUG<<"条码：[" <<barcodes<<"]cameraId:"<<cameraId;
    switch (cameraId) {
    case 0:
        {
            ui->t1->setText("条码：" + barcodes);
            ui->label->setPixmap(showPixmap);
        }
        break;
    case 1:
        {
            ui->t2->setText("条码：" + barcodes);
            ui->label_2->setPixmap(showPixmap);
        }
        break;
    case 2:
        {
            ui->t3->setText("条码：" + barcodes);
            ui->label_3->setPixmap(showPixmap);
        }
        break;
    case 3:
        {
            ui->t4->setText("条码：" + barcodes);
            ui->label_4->setPixmap(showPixmap);
        }
        break;
    default:
        break;
    }
    // 保存图片到文件夹
    if (!image.isNull()) {
        // 生成唯一的文件名：相机ID_时间戳.png
        QString timeStr = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
        QString fileName = QString("camera_%1_%2.png").arg(cameraId).arg(timeStr);
        QString filePath = m_savePath + "/" + fileName;

        // 保存图片
        if (image.save(filePath, "PNG")) {
            LOGDEBUG << "图片保存成功：" << filePath;
        } else {
            LOGDEBUG << "图片保存失败：" << filePath;
        }
    }
}

void Widget::pb_Init()
{
    //枚举子网内所有设备
    memset(&m_stDevList,0,sizeof(MV_CODEREADER_DEVICE_INFO_LIST));
    int nRet = CMvCamera::EnumDevices(&m_stDevList,MV_CODEREADER_GIGE_DEVICE);
    if(MV_CODEREADER_OK != nRet)
    {
        LOGDEBUG<<"枚举相机设备失败！";
        return;
    }

    int deviceNum = m_stDevList.nDeviceNum;
    LOGDEBUG<<"相机连接数量:"<<deviceNum;
    if(deviceNum > CAMERA_NUM)
    {
        //限制为CAMERA_NUM,实际情况需要根据相机数量定义头文件中的对象数
        deviceNum = CAMERA_NUM;
    }
    for(int i=0;i<deviceNum;i++)
    {
        MV_CODEREADER_DEVICE_INFO *pDeviceInfo = m_stDevList.pDeviceInfo[i];
        QString strSerialNumber = "";
        unsigned int nCurrentIp = 0;
        if(pDeviceInfo->nTLayerType == MV_CODEREADER_GIGE_DEVICE)
        {
            strSerialNumber = (char*)pDeviceInfo->SpecialInfo.stGigEInfo.chSerialNumber;
            nCurrentIp = pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp;
        }
        else if(pDeviceInfo->nTLayerType == MV_CODEREADER_USB_DEVICE)
        {
            strSerialNumber = (char*)pDeviceInfo->SpecialInfo.stUsb3VInfo.chSerialNumber;
        }
        else
        {
            LOGDEBUG<<"警告,未知设备枚举！";
            return;
        }
        LOGDEBUG<<"i:"<<i<<"   strSerialNumber:"<<strSerialNumber<< " now IP:"<<nCurrentIp;
        int nIp1 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
        int nIp2 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
        int nIp3 = ((pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
        int nIp4 = (pDeviceInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);
        LOGDEBUG<<"DeviceIP============="<<nIp1<<nIp2<<nIp3<<nIp4;
        QString IP = QString::number(nIp1)+"."+QString::number(nIp2)+"."+QString::number(nIp3)+"."+QString::number(nIp4);
        if(i == 0)
        {
            ui->c1->setText(IP);
        }
        else if(i == 1)
        {
            ui->c2->setText(strSerialNumber);
        }
        else if(i == 2)
        {
            ui->c3->setText(strSerialNumber);
        }
        else if(i == 4)
        {
            ui->c4->setText(strSerialNumber);
        }

        //根据相机序列号指定相机对象,就可以指定某个窗口进行显示
        //if(strSerialNumber == "DA0333897")
        //{
        //    m_deviceInfo[0] = pDeviceInfo;
        //}
        //else if(strSerialNumber == "DA0424312")
        //{
        //    m_deviceInfo[1] = pDeviceInfo;
        //}

        //不指定
        m_deviceInfo[i] = pDeviceInfo;

        //打开相机
        int nRet = m_myCamera[i]->Open(m_deviceInfo[i]);
        if(MV_CODEREADER_OK != nRet)
        {
            LOGDEBUG<<"i:"<<i<<"打开相机失败！";
            return;
        }
        else
        {
            //关闭触发模式
//            nRet = m_myCamera[i]->SetEnumValue("TriggerMode",0);
//            if(MV_CODEREADER_OK != nRet)
//            {
//                LOGDEBUG<<"i:"<<i<<"关闭触发模式失败！";
//                return;
//            }

            //设置线程对象中的回调函数
            m_grabThread[i]->setCameraPtr(m_myCamera[i]);
        }
    }
}


void Widget::on_pushButton_2_clicked()
{
    for(int i = 0;i < CAMERA_NUM;i++)
    {
        int nRet = m_myCamera[i]->StartGrabbing();
        if (MV_CODEREADER_OK != nRet)
        {
            LOGDEBUG<<"i:"<<i<<"开始取图失败！";
            return;
        }
    }
}

void Widget::on_pushButton_3_clicked()
{
    for(int i = 0;i < CAMERA_NUM;i++)
    {
        int nRet = m_myCamera[i]->StopGrabbing();
        if (MV_CODEREADER_OK != nRet)
        {
            LOGDEBUG<<"i:"<<i<<"停止取图失败！";
            return;
        }
    }
}
