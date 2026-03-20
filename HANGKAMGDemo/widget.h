#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QDateTime>
#include "cameramanager.h"
#include "cmvcamera.h"
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

private slots:
    void slot_recv_camereData(const QString &deviceId, const CameraData &data);//相机数据回调函数
    void pb_Init();
    void on_pushButton_2_clicked();
    void on_pushButton_3_clicked();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();

private:
    Ui::Widget *ui;

    MV_CODEREADER_DEVICE_INFO *m_deviceInfo[CAMERA_NUM];    //设备信息
    MV_CODEREADER_DEVICE_INFO_LIST m_stDevList;             //设备信息列表

private:
    QString m_savePath;             // 图片保存路径
    Cameramanager manager;          // 创建相机管理器
};
#endif // WIDGET_H
