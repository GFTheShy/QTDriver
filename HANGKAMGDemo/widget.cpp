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
    connect(ui->pushButton,&QPushButton::clicked,this,&Widget::pb_Init);

    connect(&manager,&Cameramanager::dataReceived,this,&Widget::slot_recv_camereData);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::slot_recv_camereData(const QString &deviceId, const CameraData &data)
{
    try {
        // dynamic_cast引用转换失败会抛出bad_cast异常，需捕获
        const MVCodeCameraData& subData = dynamic_cast<const MVCodeCameraData&>(data);

        //  1.处理数据
        if (!subData.pData || !subData.pstFrameInfo) return;
        LOGDEBUG<<"条码数量["<<subData.pstFrameInfo->UnparsedBcrList.pstCodeListEx2->nCodeNum<<"]";
        MV_CODEREADER_RESULT_BCR_EX2* stBcrResult = (MV_CODEREADER_RESULT_BCR_EX2*)subData.pstFrameInfo->UnparsedBcrList.pstCodeListEx2;
        if(stBcrResult)
        {
            for(unsigned int i = 0;i < subData.pstFrameInfo->UnparsedBcrList.pstCodeListEx2->nCodeNum;i++)
            {
                LOGDEBUG<<QString("主相机,条码序号:%1,条码内容:%2").arg(i).arg(QString::fromLocal8Bit(stBcrResult->stBcrInfoEx2[i].chCode));
            }
        }
        LOGDEBUG<<"主相机识别到的面单数量["<<subData.pstFrameInfo->pstWaybillList->nWaybillNum<<"]";
        //==========显示图片===
        QImage image;
        LOGDEBUG<<"图像参数有效性检查"<<subData.pstFrameInfo->nWidth<<" "<<subData.pstFrameInfo->nHeight;
        // 添加图像参数有效性检查
        if (subData.pstFrameInfo->nWidth <= 0 || subData.pstFrameInfo->nHeight <= 0) return;

        // 2. 将二进制数据转换为QImage（根据enPixelType）
        if (subData.pstFrameInfo->enPixelType == PixelType_CodeReader_Gvsp_Jpeg) {
            // 重要：JPEG 是压缩流，nFrameLen 是这一帧数据的实际字节长度
            // loadFromData 会自动完成 JPEG 解码
            bool ok = image.loadFromData(subData.pData, subData.pstFrameInfo->nFrameLen, "JPG");
            if (!ok) {
                LOGDEBUG << "JPEG 解码失败";
                return;
            }
        } else if (subData.pstFrameInfo->enPixelType == PixelType_CodeReader_Gvsp_Mono8) {
            // 深拷贝数据
            QByteArray bufferCopy((const char*)subData.pData, subData.pstFrameInfo->nWidth * subData.pstFrameInfo->nHeight);
            image = QImage(
                (const uchar*)bufferCopy.constData(),
                subData.pstFrameInfo->nWidth,
                subData.pstFrameInfo->nHeight,
                subData.pstFrameInfo->nWidth,
                QImage::Format_Grayscale8
            ).copy(); // 确保独立数据
        }
        LOGDEBUG<<"相机照片大小"+QString::number(image.width())+" "+QString::number(image.height());

        //  2.显示数据
        QPixmap showPixmap = QPixmap::fromImage(image).scaled(ui->label->size(),Qt::IgnoreAspectRatio,Qt::SmoothTransformation);
        QString barcodes;
        //提取条码出来
        if (subData.pstFrameInfo && subData.pstFrameInfo->UnparsedBcrList.pstCodeListEx2)
        {
            MV_CODEREADER_RESULT_BCR_EX2* stBcrResult = (MV_CODEREADER_RESULT_BCR_EX2*)subData.pstFrameInfo->UnparsedBcrList.pstCodeListEx2;
            if(!stBcrResult) return;
            for (unsigned int i = 0; i < stBcrResult->nCodeNum; i++)
            {
                if (i > 0) {
                    barcodes += ","; // 非首元素前添加逗号
                }
                barcodes += QString::fromLocal8Bit(stBcrResult->stBcrInfoEx2[i].chCode);
            }
        }
        LOGDEBUG<<"条码：[" <<barcodes<<"]deviceId:"<<deviceId;
        if(deviceId == ui->c1->text())
        {
            ui->t1->setText("条码：" + barcodes);
            ui->label->setPixmap(showPixmap);
        }
        else if(deviceId == ui->c2->text())
        {
            ui->t2->setText("条码：" + barcodes);
            ui->label_2->setPixmap(showPixmap);
        }
        else if(deviceId == ui->c3->text())
        {
            ui->t3->setText("条码：" + barcodes);
            ui->label_3->setPixmap(showPixmap);
        }
        else if(deviceId == ui->c4->text())
        {
            ui->t4->setText("条码：" + barcodes);
            ui->label_4->setPixmap(showPixmap);
        }

        // 保存图片到文件夹
        if (!image.isNull()) {
            // 生成唯一的文件名：相机ID_时间戳.png
            QString timeStr = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss_zzz");
            QString fileName = QString("camera_%1_%2.png").arg(deviceId).arg(timeStr);
            QString filePath = m_savePath + "/" + fileName;

            // 保存图片
            if (image.save(filePath, "PNG")) {
                LOGDEBUG << "图片保存成功：" << filePath;
            } else {
                LOGDEBUG << "图片保存失败：" << filePath;
            }
        }
    } catch (const std::bad_cast& e) {
        LOGDEBUG << "类型转换失败：" << e.what() << endl;
    }
}

//初始化，搜索相机，添加设备信息
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
    LOGDEBUG<<"相机数量:"<<deviceNum;
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
        LOGDEBUG<<"DeviceIP:"<<nIp1<<nIp2<<nIp3<<nIp4;
        QString IP = QString::number(nIp1)+"."+QString::number(nIp2)+"."+QString::number(nIp3)+"."+QString::number(nIp4);
        if(i == 0)
        {
            ui->c1->setText(strSerialNumber);
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

        //添加设备相机
        DeviceInfo HKCameraDevice;
        HKCameraDevice.deviceId = strSerialNumber;
        HKCameraDevice.cameraType = MVCodeCamera;
        int *p = new int;
        *p = nCurrentIp;
        HKCameraDevice.cameraParam.ptr = p;
        manager.addDevice(HKCameraDevice);
    }
}

//开始采集全部相机
void Widget::on_pushButton_2_clicked()
{
    QStringList deviceIDs = manager.allDeviceIds();
    for(const QString &deviced : deviceIDs)
    {
        manager.StartGrabbing(deviced);
    }
}
//停止采集全部相机
void Widget::on_pushButton_3_clicked()
{
    QStringList deviceIDs = manager.allDeviceIds();
    for(const QString &deviced : deviceIDs)
    {
        manager.StopGrabbing(deviced);
    }
}
//打开全部相机
void Widget::on_pushButton_4_clicked()
{
    QStringList deviceIDs = manager.allDeviceIds();
    for(const QString &deviced : deviceIDs)
    {
        manager.connectDevice(deviced);
    }
}
//关闭全部相机
void Widget::on_pushButton_5_clicked()
{
    QStringList deviceIDs = manager.allDeviceIds();
    for(const QString &deviced : deviceIDs)
    {
        manager.disconnectDevice(deviced);
    }
}
