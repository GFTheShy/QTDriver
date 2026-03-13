#include "rastersetting.h"
#include "ui_rastersetting.h"

// 定义一个IP地址字符串和数组处理函数或直接转换
auto stringToIpArray = [](const QString& str, uint8_t* ipArray) {
    // 1. 按 "." 拆分字符串
    QStringList list = str.split(".");

    // 2. 确保有 4 个部分
    if (list.size() == 4) {
        for (int i = 0; i < 4; ++i) {
            // 3. 将字符串转为数字
            ipArray[i] = static_cast<uint8_t>(list[i].toUInt());
        }
        return true;
    }
    return false;
};

RasterSetting::RasterSetting(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::RasterSetting)
{
    ui->setupUi(this);
    hide();
}

RasterSetting::~RasterSetting()
{
    delete ui;
}

void RasterSetting::on_ptnBackMain_clicked()
{
    hide();
    emit backMainSig();
}

/*//光栅连接设置
            uint8_t IP_ADDRESS[4] = {0};                //IP地址
            uint8_t NETMASK_ADDRESS[4] = {0};           //子网掩码
            uint8_t GATEWAY_ADDRESS[4] = {0};           //网关地址
            uint8_t FRAME_HEADER = 0x01;                // 帧头/地址码 (0x01)
            uint16_t TCP_ECHO_PORT = 1030;              // TCP 服务器端口
            //光栅基本设置
            uint16_t firmwaveDeviceVersion = 0;         //当前固件版本
            WorkStatus WorkStatusMode = WorkScaning;    //当前工作状态
            ScanMode ScanModeValue = RunningScan;       //当前扫描状态
            uint16_t LED_TOTAL = 0;                     //当前灯珠总数
            uint16_t RasterSpace = 0;                   //当前灯珠间距
            uint16_t step = 16;                         //跳扫模式下的扫描步长，步长可以根据需求调整（8, 16, 32）*/
void RasterSetting::loadRasterSlots(LoadMessage message)
{
    //加载参数到界面
    //光栅连接设置
    QString IPstr = QString::number(message.IP_ADDRESS[0]) + "." + QString::number(message.IP_ADDRESS[1]) + "." + QString::number(message.IP_ADDRESS[2]) + "." + QString::number(message.IP_ADDRESS[3]);
    QString netmaskStr = QString::number(message.NETMASK_ADDRESS[0]) + "." + QString::number(message.NETMASK_ADDRESS[1]) + "." + QString::number(message.NETMASK_ADDRESS[2]) + "." + QString::number(message.NETMASK_ADDRESS[3]);
    QString gatewayStr = QString::number(message.GATEWAY_ADDRESS[0]) + "." + QString::number(message.GATEWAY_ADDRESS[1]) + "." + QString::number(message.GATEWAY_ADDRESS[2]) + "." + QString::number(message.GATEWAY_ADDRESS[3]);
    QString TcpPortStr = QString::number(message.TCP_ECHO_PORT);
    QString DeviceAddrStr = QString::number(message.FRAME_HEADER);

    ui->lEditIPaddress->setText(IPstr);
    ui->lEditNetworkMask->setText(netmaskStr);
    ui->lEditGateWayAdress->setText(gatewayStr);
    ui->lEditTcpPort->setText(TcpPortStr);
    ui->lEditDeviceAddress->setText(DeviceAddrStr);
    //光栅基本设置
    QString firmwareStr = QString::number(message.firmwaveDeviceVersion);
    QString LedTotalStr = QString::number(message.LED_TOTAL);
    QString OneTimeStr = QString::number(message.OneTime);

    ui->labFirmwareVersion->setText(firmwareStr);
    if(message.WorkStatusMode == LoadMessage::WorkScaning)
    {
        ui->cBoxRunMode->setCurrentIndex(0);
    }
    else if(message.WorkStatusMode == LoadMessage::WorkSettingShield)
    {
        ui->cBoxRunMode->setCurrentIndex(1);
    }
    if(message.ScanModeValue == LoadMessage::RunningScan)
    {
        ui->cBoxScanMode->setCurrentIndex(0);
    }
    else if(message.ScanModeValue == LoadMessage::QuickScanMode)
    {
        ui->cBoxScanMode->setCurrentIndex(1);
    }
    ui->lEditRasterAllcount->setText(LedTotalStr);
    ui->sBoxScanStep->setValue(message.step);
    ui->lEditOneTime->setText(OneTimeStr);
}

void RasterSetting::on_ptnConnectRead_clicked()
{
    emit readConnectSig();
}

void RasterSetting::on_ptnBasicRead_clicked()
{
    emit readBasicSig();
}

void RasterSetting::on_ptnConnectWrite_clicked()
{
    QString ipStr = ui->lEditIPaddress->text();
    QString netmaskStr = ui->lEditNetworkMask->text();
    QString gatewayStr = ui->lEditGateWayAdress->text();
    LoadMessage tempMessage;
    // 执行转换
    stringToIpArray(ipStr, tempMessage.IP_ADDRESS);
    stringToIpArray(netmaskStr, tempMessage.NETMASK_ADDRESS);
    stringToIpArray(gatewayStr, tempMessage.GATEWAY_ADDRESS);
    tempMessage.TCP_ECHO_PORT = ui->lEditTcpPort->text().toUInt();
    tempMessage.FRAME_HEADER = ui->lEditDeviceAddress->text().toUShort();
    emit writeConnectSig(tempMessage);
}

void RasterSetting::on_ptnBasicWrite_clicked()
{
    LoadMessage tempMessage;
    if(ui->cBoxRunMode->currentIndex() == 0) tempMessage.WorkStatusMode = LoadMessage::WorkScaning;
    else if(ui->cBoxRunMode->currentIndex() == 1) tempMessage.WorkStatusMode = LoadMessage::WorkSettingShield;
    if(ui->cBoxScanMode->currentIndex() == 0) tempMessage.ScanModeValue = LoadMessage::RunningScan;
    else if(ui->cBoxScanMode->currentIndex() == 1) tempMessage.ScanModeValue = LoadMessage::QuickScanMode;
    tempMessage.LED_TOTAL = ui->lEditRasterAllcount->text().toUInt();
    tempMessage.step = (uint)ui->sBoxScanStep->value();
    tempMessage.OneTime = ui->lEditOneTime->text().toUInt();
    emit writeBasicSig(tempMessage);
}




