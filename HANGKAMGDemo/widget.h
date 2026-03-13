#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDateTime>
#include "grabimgthread.h"

//相机数量
#define CAMERA_NUM 4


QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

    void initWidget();
private slots:
    void slot_imageReady(const QImage &image, MV_CODEREADER_IMAGE_OUT_INFO_EX2* pstFrameInfo,int cameraId);//相机数据回调函数
    void pb_Init();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();

private:
    Ui::Widget *ui;

    CMvCamera *m_myCamera[CAMERA_NUM];             //相机对象
    GrabImgThread *m_grabThread[CAMERA_NUM];       //捕获图像线程
    MV_CODEREADER_DEVICE_INFO *m_deviceInfo[CAMERA_NUM];   //设备信息
    MV_CODEREADER_DEVICE_INFO_LIST m_stDevList;   //设备信息列表

private:
    QString m_savePath; // 图片保存路径
};
#endif // WIDGET_H
