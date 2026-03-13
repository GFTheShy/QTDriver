#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
    tcpSocket = new QTcpSocket(this);
    RasterSettingUI = new RasterSetting();
    connect(RasterSettingUI,&RasterSetting::backMainSig,this,&Widget::backMainSlots);
    connect(this,&Widget::loadRasterSig,RasterSettingUI,&RasterSetting::loadRasterSlots);
    connect(RasterSettingUI,&RasterSetting::readConnectSig,this,&Widget::readConnectSlots);
    connect(RasterSettingUI,&RasterSetting::readBasicSig,this,&Widget::readBasicSlots);
    connect(RasterSettingUI,&RasterSetting::writeConnectSig,this,&Widget::writeConnectSlots);
    connect(RasterSettingUI,&RasterSetting::writeBasicSig,this,&Widget::writeBasicSlots);

    initLedGrid();

    connect(tcpSocket, &QTcpSocket::readyRead, this, &Widget::onReadyRead);
    connect(tcpSocket, &QTcpSocket::connected, this, &Widget::onSocketConnected);
    connect(tcpSocket, &QTcpSocket::disconnected, this, &Widget::onSocketDisconnected);

    //帧率计算相关
    m_fpsDisplayTimer.setInterval(1000);
    connect(&m_fpsDisplayTimer, &QTimer::timeout, this, &Widget::updateFpsDisplay);

    //界面刷新相关
    m_uiRefreshTimer = new QTimer(this);
    connect(m_uiRefreshTimer, &QTimer::timeout, this, &Widget::refreshUI);
    m_uiRefreshTimer->start(33); // 约 30 FPS，非常流畅且不卡顿
}

Widget::~Widget() { delete ui; }

void Widget::initLedGrid() {
    for (uint i = 0; i < LED_TOTAL; ++i) {
        leds[i].label = new QLabel();
        leds[i].label->setFixedSize(20, 20);
        leds[i].label->setProperty("id", i);

        leds[i].label->setText(QString::number(i));// 设置显示的序号文本

        leds[i].label->setAlignment(Qt::AlignCenter);// 设置文本居中对齐
        QFont font = leds[i].label->font();// 固定字号
        font.setPointSize(6);
        // 设置字体加粗
        font.setBold(true);
        leds[i].label->setFont(font);

        leds[i].label->installEventFilter(this); // 方便通过点击屏蔽
        updateLedStyle(i);

        // 16列布局
        ui->gridLayoutLeds->addWidget(leds[i].label, i / 16, i % 16);
    }
}

//连接成功后读取光栅的参数并加载到界面以及设置界面
void Widget::loadRaster()
{
    //帧率相关
    m_frameCount = 0;
    m_fpsTimer.restart();
    m_fpsDisplayTimer.start();
    ui->dSBoxRasterSpace->setValue(RasterSpace);
    // 读取光栅的参数并加载到界面以及设置界面
    // 发送一系列查询指令 (根据协议，读取指令的数据长度通常为4，即只有 Head+Func+LenH+LenL)

    // 获取工作模式 (主动/应答)
    QByteArray readPayload;
    readPayload.append((char)0xFF);
    sendCommand(0xA0, readPayload); // [cite: 1]

    // 获取扫描模式 (全扫/跳扫)
    readPayload.clear();
    readPayload.append((char)0xFF);
    sendCommand(0xB5, readPayload); // [cite: 3]

    //获取灯珠的屏蔽状态
    sendCommand(0xA7, QByteArray()); // [cite: 3]

}

// 模拟灯珠点击屏蔽功能 (对应 0xA2 指令)
bool Widget::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::MouseButtonDblClick) {
        qDebug()<<"屏蔽灯珠eventFilter";
        QLabel* lbl = qobject_cast<QLabel*>(watched);
        if (lbl) {
            int id = lbl->property("id").toInt();
            leds[id].isShielded = !leds[id].isShielded;
            qDebug()<<"屏蔽灯珠"<<id;
            // 构造 0xA2 屏蔽指令数据包
            QByteArray payload;
            payload.append((char)(id >> 8));    // Addr H
            payload.append((char)(id & 0xFF));  // Addr L
            payload.append((char)(leds[id].isShielded ? 0x01 : 0x00));
            sendCommand(0xA2, payload);

            updateLedStyle(id);
            return true;
        }
    }
    return QWidget::eventFilter(watched, event);
}

void Widget::updateLedStyle(int i) {
    QString style = "border-radius: 10px; border: 1px solid #444;";
    if (leds[i].isShielded) {
        style += "background-color: yellow;";
    } else if (leds[i].isBlocked) {
        style += "background-color: red;";
    } else {
        style += "background-color: #00FF00;"; // 绿色
    }
    leds[i].label->setStyleSheet(style);
}

void Widget::onReadyRead() {
    QByteArray data = tcpSocket->readAll();
    //qDebug() << "收到原始字节数：" << data.size() << "内容：" << data.toHex(' ');
    // 1. 将新到的数据追加到缓冲区
    m_buffer.append(data);
    //qDebug()<<"读取数据"<<m_buffer;
    // 2. 循环处理缓冲区，直到没有完整的帧为止
    // 最小帧长度通常为：帧头(1) + 功能码(1) + 长度(2) = 4字节
    while (m_buffer.size() >= 4) {
        // 检查帧头（默认协议地址码/帧头为 0x01）
        if (static_cast<uint8_t>(m_buffer[0]) != FRAME_HEAD) {
            m_buffer.remove(0, 1); // 如果不是帧头，丢弃该字节继续寻找
            continue;
        }

        // 解析长度字段（高字节在索引2，低字节在索引3）
        uint16_t frameLen = (static_cast<uint8_t>(m_buffer[2]) << 8) | static_cast<uint8_t>(m_buffer[3]);

        // 3. 检查当前缓冲区的数据是否足够一整帧
        if (m_buffer.size() < frameLen) {
            // 数据还没收全（半包），跳出循环，等待下一次 readyRead
            break;
        }

        // 4. 提取出完整的一帧
        QByteArray completeFrame = m_buffer.left(frameLen);

        // 5. 调用原来的解析函数
        parseProtocol(completeFrame);

        // 6. 从缓冲区中删除已经处理掉的这一帧
        m_buffer.remove(0, frameLen);
    }
}

void Widget::parseProtocol(const QByteArray &frame) {
    if(frame.isEmpty()) return;
    uint8_t func = (uint8_t)frame[1];
    unsigned int len = ((uint8_t)frame[2] << 8) | (uint8_t)frame[3];
    const uint8_t* data = (const uint8_t*)frame.data();
    //qDebug()<<"解析数据："<<func<< len;

    switch (func)
    {
        case 0xA1:
            {
                //qDebug()<<"解析灯珠数据";
                if(len == (6+((LED_TOTAL + 7) / 8)))    //全扫模式需要通过灯珠数量计算帧长
                {
                    // 灯珠状态数据
                        //qDebug()<<"灯珠状态数据";
                        uint8_t FrameHead[6];
                        memcpy(FrameHead,frame.data(),6);
                        unsigned int EncodeValue = (FrameHead[4] << 8)+FrameHead[5];
                        //qDebug()<<"编码器数值:"<<EncodeValue;
                        ui->EncoderValueLabel->setText(QString::number(EncodeValue));//更新编码器数值显示

                        const uint8_t* ptr = (const uint8_t*)frame.data() + 6; // 跳过Head+Func+Len+Encoder
                        int lowPoint = -1;  // 记录第一个被遮挡的索引
                        int highPoint = 0; // 记录最后一个被遮挡的索引
                        float blockedLength = 0; // 记录遮光长度
                        int blockedTotal = 0; // 记录遮光数量
                        for (uint i = 0; i < LED_TOTAL; ++i) {
                            bool blocked = (ptr[i/8] >> (i%8)) & 0x01;
                            leds[i].isBlocked = blocked;
                            if (blocked)
                            {
                                // 如果未被屏蔽，才计算
                                if (!leds[i].isShielded) {
                                    blockedTotal++;
                                    // 计算低点：记录第一次发现遮挡的位置
                                    if (lowPoint == -1) lowPoint = i;
                                    // 计算高点：不断更新，最后一次更新即为最高点
                                    highPoint = i;
                                }
                            }
                            m_latestBlockedStatus[i] = (ptr[i/8] >> (i%8)) & 0x01;
                        }
                        if(lowPoint == -1)
                        {
                            ui->BlackLowValueLabel->setText(QString::number(0));        //遮光低点界面显示
                        }
                        else
                        {
                            ui->BlackLowValueLabel->setText(QString::number(lowPoint)); //遮光低点界面显示
                        }

                        ui->BlackHighValueLabel->setText(QString::number(highPoint));   //遮光高点界面显示
                        blockedLength = (highPoint - lowPoint + 1) * RasterSpace;       //计算遮光长度
                        ui->BlackLengthValueLabel->setText(QString::number(blockedLength) + " mm");
                        ui->lblBlockedCount->setText(QString("%1").arg(blockedTotal));  //更新遮光数量显示
                    m_frameCount++;//帧计数
                }
                else if(len == 10)   //跳扫模式，只有遮光低点和高点和编码器值，不能更新界面
                {
                    // 跳扫模式逻辑
                    qDebug()<<"跳扫模式灯珠状态数据";
                    uint8_t FrameHead[10];
                    memcpy(FrameHead,frame.data(),10);
                    unsigned int EncodeValue = (FrameHead[4] << 8)+FrameHead[5];
                    unsigned int LowAddr = (FrameHead[6] << 8)+FrameHead[7];
                    unsigned int HighAddr = (FrameHead[8] << 8)+FrameHead[9];
                    qDebug()<<"编码器数值:"<<EncodeValue;
                    ui->EncoderValueLabel->setText(QString::number(EncodeValue));//更新编码器数值显示
                    float blockedLength = (HighAddr - LowAddr + 1) * RasterSpace; // 记录遮光长度
                    // 灯珠状态数据
                    ui->BlackLowValueLabel->setText(QString::number(LowAddr));//遮光低点界面显示
                    ui->BlackHighValueLabel->setText(QString::number(HighAddr));//遮光高点界面显示
                    ui->BlackLengthValueLabel->setText(QString::number(blockedLength) + " mm");
                    ui->lblBlockedCount->setText("xx");//更新遮光数量显示
                    m_frameCount++;//帧计数
                }
            }
            break;
        case 0xA0: // 工作模式响应 [cite: 1]
                {
                    qDebug()<<"工作模式响应";
                    if(len >= 5)
                    {
                        if(data[4] == 0x00)
                        {
                            msg.WorkStatusMode =  LoadMessage::WorkScaning;
                            ui->label_10->setText("主动模式");
                        }
                        else
                        {
                            msg.WorkStatusMode =  LoadMessage::WorkSettingShield;
                            ui->label_10->setText("应答模式");
                        }

                    }
                    emit loadRasterSig(msg);
                }
            break;
        case 0xA7: // 灯珠的屏蔽状态 [cite: 1]
                {
                    qDebug()<<"灯珠的屏蔽状态响应";
                    if(len == 68)
                    {
                        const uint8_t* ptr = (const uint8_t*)frame.data() + 4; // 跳过帧头和长度位
                        for (uint i = 0; i < LED_TOTAL; ++i) {
                            // 按照协议位图逻辑解析：第 i 个灯珠在第 i/8 字节的 i%8 位
                            bool shielded = (ptr[i / 8] >> (i % 8)) & 0x01;
                            leds[i].isShielded = shielded;

                            // 更新 UI 样式，屏蔽的灯珠会变黄
                            updateLedStyle(i);
                        }
                        qDebug() << "硬件屏蔽状态已同步到界面";
                    }
                }

               break;
        case 0xB0: // IP地址 [cite: 1]
                {
                    qDebug()<<"IP地址响应";
                    if(len == 8) memcpy(msg.IP_ADDRESS, data + 4, 4);
                    emit loadRasterSig(msg);
                }

               break;

        case 0xB1: // 子网掩码 [cite: 2]
                {
                    qDebug()<<"子网掩码响应";
                    if(len == 8) memcpy(msg.NETMASK_ADDRESS, data + 4, 4);
                    emit loadRasterSig(msg);
                }

            break;

        case 0xB2: // 网关地址 [cite: 2]
                {
                    qDebug()<<"网关地址响应";
                    if(len == 8) memcpy(msg.GATEWAY_ADDRESS, data + 4, 4);
                    emit loadRasterSig(msg);
                }
            break;

        case 0xB3: // 端口号 [cite: 2]
                {
                    qDebug()<<"端口号响应";
                    if(len == 6) msg.TCP_ECHO_PORT = (data[4] << 8) | data[5];
                    emit loadRasterSig(msg);
                }

            break;

        case 0xB4: // 地址位 [cite: 2]
                {
                    qDebug()<<"地址位响应";
                    if(len == 5) msg.FRAME_HEADER = data[4];
                    emit loadRasterSig(msg);
                }

            break;

        case 0xB5: // 扫描模式 [cite: 3]
                {
                    qDebug()<<"扫描模式响应";
                    if(data[4] == 0x00)
                    {
                        msg.ScanModeValue =  LoadMessage::RunningScan;
                        ui->label_13->setText("全扫模式");
                    }
                    else
                    {
                        msg.ScanModeValue =  LoadMessage::QuickScanMode;
                        ui->label_13->setText("跳扫模式");
                    }
                    emit loadRasterSig(msg);
                }
            break;

        case 0xB6: // 跳扫步长 [cite: 3]
                {
                    qDebug()<<"跳扫步长响应";
                    if(len == 6) msg.step = (data[4] << 8) | data[5];
                    emit loadRasterSig(msg);
                }

            break;

        case 0xC0: // 固件版本 [cite: 3]
                {
                    if(len == 5) msg.firmwaveDeviceVersion = data[4];
                    qDebug()<<"固件版本响应: " << msg.firmwaveDeviceVersion;
                    emit loadRasterSig(msg);
                }
            break;
        case 0xC1: // 灯珠总数 [cite: 3]
                {
                    if(len == 6) msg.LED_TOTAL = (data[4] << 8) | data[5];
                    qDebug()<<"灯珠总数响应: " << msg.LED_TOTAL;
                    emit loadRasterSig(msg);
                }
            break;
        case 0xC2: // 灯珠检测时间 [cite: 3]
                {
                    if(len == 6) msg.OneTime = (data[4] << 8) | data[5];
                    qDebug()<<"灯珠检测时间响应: " << msg.OneTime;
                    emit loadRasterSig(msg);
                }
            break;
        case 0xFF:
            if(len == 5)
            {
                qDebug()<<"错误码响应";
                uint8_t errcode = data[4];
                switch (errcode) {
                    case 0xFF://无异常，正常状态
                    {qDebug()<<"无异常，正常状态";}
                        break;
                    case 0x00://写入失败
                    {qDebug()<<"写入失败";}
                        break;
                    case 0x01://未知功能码
                    {qDebug()<<"未知功能码";}
                        break;
                default:
                    {qDebug()<<"未定义错误码";}
                    break;
                }
            }
            break;
    default:
        qDebug()<<"未知响应";
        break;
    }

}

void Widget::sendCommand(uint8_t func, const QByteArray &payload) {
    if (tcpSocket->state() != QAbstractSocket::ConnectedState) return;
    QByteArray pkg;
    pkg.append((char)FRAME_HEAD);
    pkg.append((char)func);
    uint16_t len = payload.size() + 4;
    pkg.append((char)(len >> 8));
    pkg.append((char)(len & 0xFF));
    pkg.append(payload);
    tcpSocket->write(pkg);
    qDebug()<<"发送数据："<<pkg;
    tcpSocket->flush(); // 强制发送
}

void Widget::on_btnConnect_clicked() {
    if (tcpSocket->state() == QAbstractSocket::UnconnectedState) {
        QString ip = ui->ipEdit->text();
        quint16 port = ui->portEdit->text().toUShort();
        // 确保 QTcpSocket 尝试直接连接到 IP 地址，而不是通过系统或环境变量配置的代理。
        tcpSocket->setProxy(QNetworkProxy::NoProxy);
        tcpSocket->connectToHost(QHostAddress(ip), port);
        qDebug()<<ui->ipEdit->text()<< ui->portEdit->text().toUShort();
    } else {
        tcpSocket->disconnectFromHost();
    }
}

void Widget::on_btnClearShield_clicked() {
    sendCommand(0xA4, QByteArray()); // 清除屏蔽指令
    for(uint i=0; i<LED_TOTAL; i++) {
        leds[i].isShielded = false;
        updateLedStyle(i);
    }
}

void Widget::onSocketConnected() {
    ui->lblStatus->setText("状态: 已连接");
    ui->btnConnect->setText("断开");
    loadRaster(); //连接成功后读取光栅的参数并加载到界面以及设置界面
}

void Widget::onSocketDisconnected() {
    ui->lblStatus->setText("状态: 已断开");
    ui->btnConnect->setText("连接设备");
}

void Widget::updateFpsDisplay()
{
    // 更新 FPS 显示
    ui->FPSValueLabel->setText(QString("%1 FPS").arg(m_frameCount));

    // 重置帧计数
    m_frameCount = 0;
}

void Widget::refreshUI()
{
    // 只有在数据真正变化或需要刷新时才操作 UI
    for (uint i = 0; i < LED_TOTAL; ++i) {
        leds[i].isBlocked = m_latestBlockedStatus[i];
        updateLedStyle(i);
    }
}

// 屏蔽当前所有处于遮光状态的灯珠
void Widget::on_btnBlackShield_clicked()
{
    // 初始化payload为全0（0=不屏蔽，1=屏蔽）
    QByteArray payload(STATE_BYTES, 0x00);

    // 遍历所有灯珠，设置对应屏蔽位
    for (uint i = 0; i < LED_TOTAL; ++i) {
        // 仅处理：处于遮光状态 且 未被屏蔽的灯珠
        if (leds[i].isBlocked && !leds[i].isShielded) {
            int byteIndex = i / 8;    // 灯珠对应的数据字节索引
            int bitPos = i % 8;       // 字节内的位索引（0~7，需与硬件协议位序一致）

            // 彻底解决QByteRef和类型转换问题：取原始指针操作
            unsigned char* dataPtr = reinterpret_cast<unsigned char*>(payload.data());
            dataPtr[byteIndex] |= (1 << bitPos); // 设置对应位为1（屏蔽状态）

            // 更新本地灯珠状态
            leds[i].isShielded = true;
        }
    }

    // 直接使用项目现有sendCommand函数发送0xA3功能码指令
    sendCommand(0xA3, payload);

    // 更新所有灯珠的显示样式
    for (uint i = 0; i < LED_TOTAL; ++i) {
        updateLedStyle(i);
    }

    qDebug() << "已批量设置遮光状态灯珠为屏蔽状态，数据字节数：" << STATE_BYTES;
}

//跳转光栅设置界面
void Widget::on_btnRasterSetting_clicked()
{
    qDebug() << "跳转光栅设置界面";
    // 同步位置和大小：将主窗口的几何信息设置给设置界面
    RasterSettingUI->setGeometry(this->geometry());
    this->hide();
    RasterSettingUI->show();
}

void Widget::backMainSlots()
{
    this->show();
}

void Widget::readConnectSlots()
{
    // 获取IP、子网掩码、网关、端口、地址位 (长度为4表示读)
    sendCommand(0xB0, QByteArray()); // IP [cite: 1]
    sendCommand(0xB1, QByteArray()); // 子网掩码 [cite: 2]
    sendCommand(0xB2, QByteArray()); // 网关 [cite: 2]
    sendCommand(0xB3, QByteArray()); // 端口 [cite: 2]
    sendCommand(0xB4, QByteArray()); // 地址位 [cite: 2]
}

void Widget::readBasicSlots()
{
    QByteArray payload;
    payload.append((char)0xFF);
    sendCommand(0xA0, payload); // [cite: 1]

    // 获取扫描模式 (全扫/跳扫)
    payload.clear();
    payload.append((char)0xFF);
    sendCommand(0xB5, payload); // [cite: 3]

    // 获取跳扫步长
    sendCommand(0xB6, QByteArray()); // [cite: 3]

    // 获取固件版本
    sendCommand(0xC0, QByteArray()); // [cite: 3]

    // 获取灯珠总数
    sendCommand(0xC1, QByteArray()); // [cite: 3]

    QTimer::singleShot(100,[this]{
        // 获取单个灯珠检测时间
        sendCommand(0xC2, QByteArray()); // [cite: 3]
    });

}

void Widget::writeConnectSlots(LoadMessage TMessage)
{
    QByteArray payload;
    //写入IP地址
    payload.clear();
    payload.append((char)TMessage.IP_ADDRESS[0]);
    payload.append((char)TMessage.IP_ADDRESS[1]);
    payload.append((char)TMessage.IP_ADDRESS[2]);
    payload.append((char)TMessage.IP_ADDRESS[3]);
    sendCommand(0xB0, payload);
    //写入子网掩码
    payload.clear();
    payload.append((char)TMessage.NETMASK_ADDRESS[0]);
    payload.append((char)TMessage.NETMASK_ADDRESS[1]);
    payload.append((char)TMessage.NETMASK_ADDRESS[2]);
    payload.append((char)TMessage.NETMASK_ADDRESS[3]);
    sendCommand(0xB1, payload);
    //写入网关地址
    payload.clear();
    payload.append((char)TMessage.GATEWAY_ADDRESS[0]);
    payload.append((char)TMessage.GATEWAY_ADDRESS[1]);
    payload.append((char)TMessage.GATEWAY_ADDRESS[2]);
    payload.append((char)TMessage.GATEWAY_ADDRESS[3]);
    sendCommand(0xB2, payload);
    //写入端口号
    payload.clear();
    payload.append((char)(TMessage.TCP_ECHO_PORT >> 8));
    payload.append((char)(TMessage.TCP_ECHO_PORT & 0xFF));
    sendCommand(0xB3, payload);
    //写入设备地址
    payload.clear();
    payload.append((char)TMessage.FRAME_HEADER);
    sendCommand(0xB4, payload);
}

void Widget::writeBasicSlots(LoadMessage TMessage)
{
    QByteArray payload;
    //写入运行模式
    payload.clear();
    if(TMessage.WorkStatusMode == LoadMessage::WorkScaning) payload.append((char)0x00);
    else if(TMessage.WorkStatusMode == LoadMessage::WorkSettingShield) payload.append((char)0x01);
    sendCommand(0xA0, payload);
    //写入扫描模式
    payload.clear();
    if(TMessage.ScanModeValue == LoadMessage::RunningScan) payload.append((char)0x00);
    else if(TMessage.ScanModeValue == LoadMessage::QuickScanMode) payload.append((char)0x01);
    sendCommand(0xB5, payload);
    //写入灯珠总数
    payload.clear();
    payload.append((char)(TMessage.LED_TOTAL >> 8));
    payload.append((char)(TMessage.LED_TOTAL & 0xFF));
    sendCommand(0xC1, payload);
    //写入跳扫扫描步长
    payload.clear();
    payload.append((char)(TMessage.step >> 8));
    payload.append((char)(TMessage.step & 0xFF));
    sendCommand(0xB6, payload);
    //写入每个灯珠的扫描时间
    payload.clear();
    payload.append((char)(TMessage.OneTime >> 8));
    payload.append((char)(TMessage.OneTime & 0xFF));
    sendCommand(0xC2, payload);
}

void Widget::on_dSBoxRasterSpace_valueChanged(double arg1)
{
    RasterSpace = arg1;
}

void Widget::on_pushButton_clicked()
{
    //应答模式下主动获取一帧数据
    if( msg.WorkStatusMode ==  LoadMessage::WorkSettingShield)
    {
        //应答模式下主动获取一帧数据
        sendCommand(0xA1, QByteArray()); // [cite: 3]
    }
}
