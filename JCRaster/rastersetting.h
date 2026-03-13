#ifndef RASTERSETTING_H
#define RASTERSETTING_H

#include "main.h"

namespace Ui {
class RasterSetting;
}

class RasterSetting : public QWidget
{
    Q_OBJECT

public:
    explicit RasterSetting(QWidget *parent = nullptr);
    ~RasterSetting();
signals:
    void backMainSig();//返回主界面
    void readConnectSig();//读取连接界面
    void readBasicSig();//读取基本界面
    void writeConnectSig(LoadMessage TMessage);//写入连接界面
    void writeBasicSig(LoadMessage TMessage);//写入连接界面
private slots:
    void on_ptnBackMain_clicked();
    void on_ptnConnectRead_clicked();

    void on_ptnBasicRead_clicked();

    void on_ptnConnectWrite_clicked();

    void on_ptnBasicWrite_clicked();

public slots:
    void loadRasterSlots(LoadMessage message);//加载硬件信息
private:
    Ui::RasterSetting *ui;

};

#endif // RASTERSETTING_H
