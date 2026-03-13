#ifndef WIDGET_H
#define WIDGET_H

#include "main.h"
#include "rastersetting.h"
QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

struct LedNode {
    bool isBlocked = false;
    bool isShielded = false;
    QLabel* label = nullptr;
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

signals:
    void loadRasterSig(LoadMessage message);//加载光栅硬件信息
private slots:
    void on_btnConnect_clicked();
    void on_btnClearShield_clicked();
    void onReadyRead();
    void onSocketConnected();
    void onSocketDisconnected();
    void updateFpsDisplay();            // FPS 计时器槽函数
    void refreshUI();                   // 界面刷新函数
    void on_btnBlackShield_clicked();
    void on_btnRasterSetting_clicked();
    void backMainSlots();
    void readConnectSlots();//读取连接界面
    void readBasicSlots();//读取基本界面
    void writeConnectSlots(LoadMessage TMessage);//写入连接界面
    void writeBasicSlots(LoadMessage TMessage);//写入连接界面
    void on_dSBoxRasterSpace_valueChanged(double arg1);
    void on_pushButton_clicked();

private:
    Ui::Widget *ui;
    QTcpSocket *tcpSocket;
    RasterSetting *RasterSettingUI;

    LoadMessage msg;// 定义一个临时的 message 用于信号传递
    unsigned int LED_TOTAL = 448;//当前灯珠数量
    unsigned int FRAME_HEAD = 0x01;//帧头
    float RasterSpace = 2.0;//默认灯珠间距2mm
    // 灯珠状态所需字节数：(灯珠数量 + 7) / 8 （向上取整，兼容非8倍数的灯珠数量）
    const int STATE_BYTES = (LED_TOTAL + 7) / 8;
    LedNode leds[LED_MAXTOTAL]; //灯珠列表
    QByteArray m_buffer; // 增加接收缓冲区，处理粘包和半包
    QTimer *m_uiRefreshTimer;   //UI刷新定时器
    bool m_latestBlockedStatus[LED_MAXTOTAL];// 存储最新的灯珠状态

    void initLedGrid(); //初始化灯珠界面
    void loadRaster();  //连接成功后读取光栅的参数并加载到界面以及设置界面
    void parseProtocol(const QByteArray &data);     //解析数据
    void sendCommand(uint8_t func, const QByteArray &payload);//发送数据给服务器
    void updateLedStyle(int index);
    bool eventFilter(QObject *watched, QEvent *event) override;

    //帧率相关
    int m_frameCount = 0;   //帧率计数
    QTimer m_fpsDisplayTimer;
    QElapsedTimer m_fpsTimer;
};
#endif // WIDGET_H
