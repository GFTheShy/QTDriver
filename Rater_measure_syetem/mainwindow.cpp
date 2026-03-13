#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow) {
    ui->setupUi(this);
    // 初始化 Vector 大小，使其与之前的数组长度一致
    WidthLeds.resize(LED_MAXTOTAL);
    HeightLeds.resize(LED_MAXTOTAL);
    connect(this, &MainWindow::RastervolumeData,this,&MainWindow::handVolumeInfo);
    // 初始化两个光栅
    initDevice("192.168.10.3", 1030, 1);
    initDevice("192.168.10.4", 1030, 2);
}

MainWindow::~MainWindow() {
    // 优雅退出线程
    if(thread1) { thread1->quit(); thread1->wait(); }
    if(thread2) { thread2->quit(); thread2->wait(); }
    delete ui;
}

void MainWindow::initDevice(const QString &ip, int port, int id) {
    QThread *thread = new QThread(this);
    LightCurtainWorker *worker = new LightCurtainWorker(ip, port,LED_TOTAL);
    worker->moveToThread(thread);

    if (id == 1) { thread1 = thread; worker1 = worker; }
    else { thread2 = thread; worker2 = worker; }

    // 信号槽连接
    connect(thread, &QThread::started, worker, &LightCurtainWorker::connectToDevice);
    if (id == 1)
    {
        connect(worker, &LightCurtainWorker::dataParsed, this, &MainWindow::onWidthIP_DataReceived);
        connect(worker, &LightCurtainWorker::ShieldedParsed, this, &MainWindow::onWidthIP_ShieldedParsed);
        connect(this, &MainWindow::SetWidthRasterParameter,worker, &LightCurtainWorker::SetRasterParameter);
    }
    else
    {
        connect(worker, &LightCurtainWorker::dataParsed, this, &MainWindow::onHeightIP_DataReceived);
        connect(worker, &LightCurtainWorker::ShieldedParsed, this, &MainWindow::onHeightIP_ShieldedParsed);
        connect(this, &MainWindow::SetHeightRasterParameter,worker, &LightCurtainWorker::SetRasterParameter);
    }

    connect(this, &MainWindow::GetRasterParameter,worker, &LightCurtainWorker::GetRasterParameter);
    connect(worker, &LightCurtainWorker::connectionStatusChanged, this, &MainWindow::updateStatus);

    // 线程销毁时自动删除 worker
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);

    thread->start();
}

MainWindow::ViewContext MainWindow::computeFitTransform(const QRectF &dataBounds, int imgWidth, int imgHeight, double padding, bool flipX, bool flipY)
{
    ViewContext ctx;
    ctx.screenRect = QRectF(0, 0, imgWidth, imgHeight);

    // 1. 确保数据边界有效，避免除零错误
    double w = dataBounds.width();
    double h = dataBounds.height();
    if (w < 1.0) w = 1.0;
    if (h < 1.0) h = 1.0;

    // 2. 计算保持长宽比的缩放比例
    // 可用区域 = 图像尺寸 - 双倍边距
    double availableW = imgWidth - 2 * padding;
    double availableH = imgHeight - 2 * padding;
    double scaleX = availableW / w;
    double scaleY = availableH / h;

    // 取较小的缩放比例，确保物体能完整显示
    ctx.scale = qMin(scaleX, scaleY);

    // 3. 构建变换矩阵 (从下往上阅读逻辑)
    QTransform t;

    // (Step 3) 移动到画布中心
    t.translate(imgWidth / 2.0, imgHeight / 2.0);

    // (Step 2) 缩放与翻转
    // 如果 flipX 为真 (如俯视图可能需要镜像 X)，则 X 轴乘以负的缩放比
    // 如果 flipY 为真 (如侧视图物理Y向上，屏幕Y向下)，则 Y 轴乘以负的缩放比
    t.scale(flipX ? -ctx.scale : ctx.scale, flipY ? -ctx.scale : ctx.scale);

    // (Step 1) 将数据中心移动到原点 (0,0)
    t.translate(-dataBounds.center().x(), -dataBounds.center().y());

    ctx.matrix = t;
    return ctx;
}

void MainWindow::drawSmartRuler(QPainter &painter, QPointF p1, QPointF p2, double actualValueMM, const QColor &color)
{
    // 忽略太小的尺寸
    if (actualValueMM < 1.0) return;

    painter.save(); // 保存画笔状态

    // --- 1. 设置样式 ---
    QPen pen(color, 2);
    painter.setPen(pen);
    painter.setFont(QFont("Arial", 12, QFont::Bold));

    // --- 2. 几何计算 ---
    QLineF line(p1, p2);
    double length = line.length();
    if (length < 5.0) { painter.restore(); return; }

    QLineF normal = line.normalVector();
    QPointF offsetDir = (normal.p2() - normal.p1()) / normal.length();

    double offsetDist = 20.0;
    QPointF p1_off = p1 + offsetDir * offsetDist;
    QPointF p2_off = p2 + offsetDir * offsetDist;
    QPointF midPoint = (p1_off + p2_off) / 2.0;

    // --- 3. 绘制线条 ---
    int offValue = 5;// 线条两边顶点垂直的长度
    QPointF tickStart1 = p1 + offsetDir * (offsetDist - offValue);
    QPointF tickEnd1   = p1 + offsetDir * (offsetDist + offValue);
    painter.drawLine(tickStart1, tickEnd1);

    QPointF tickStart2 = p2 + offsetDir * (offsetDist - offValue);
    QPointF tickEnd2   = p2 + offsetDir * (offsetDist + offValue);
    painter.drawLine(tickStart2, tickEnd2);

    painter.drawLine(p1_off, p2_off);

    // --- 4. 绘制文字 ---
    QString text = QString::number(actualValueMM, 'f', 0) + " mm";
    QFontMetrics fm(painter.font());

    //  将 horizontalAdvance 改为 width (旧版 Qt 兼容)
    int textW = fm.width(text);
    int textH = fm.height();

    painter.translate(midPoint);

    double angle = line.angle();
    double rotateAngle = -angle;

    if (angle > 90 && angle < 270) {
        rotateAngle += 180;
    }

    painter.rotate(rotateAngle);
    painter.drawText(-textW / 2, -5, text);

    painter.restore();
}

void MainWindow::checkShieldingStatus(QVector<LedNode> &leds, unsigned int total, const QString &type)
{
    int activeShieldedCount = 0;   // 有效区域内的屏蔽总数
        int currentContinuous = 0;     // 当前连续计数
        int maxContinuousFound = 0;    // 发现的最大连续数

        // 确定有效检测范围
        uint startIndex = SAFE_ZONE_START;
        uint endIndex = total - SAFE_ZONE_END;

        if (endIndex <= startIndex) return; // 防止配置错误导致越界

        for (uint i = startIndex; i < endIndex; ++i) {
            if (leds[i].isShielded) {
                activeShieldedCount++;
                currentContinuous++;
                if (currentContinuous > maxContinuousFound) {
                    maxContinuousFound = currentContinuous;
                }
            } else {
                currentContinuous = 0; // 遇到正常灯珠，计数器复位
            }
        }

        // --- 报警逻辑判定 ---
        QStringList errorReasons;

        if (activeShieldedCount > MAX_SHIELDED_LIMIT) {
            errorReasons << QString("有效检测区屏蔽灯珠过多(%1个)").arg(activeShieldedCount);
        }

        if (maxContinuousFound >= CONTINUOUS_THRESHOLD) {
            errorReasons << QString("发现连续%1个灯珠被屏蔽(可能存在较大污渍或硬件损坏)").arg(maxContinuousFound);
        }

        // --- 执行报警 ---
        if (!errorReasons.isEmpty()) {
            QString fullMsg = QString("【%1光栅警告】\n检测到当前硬件误差较大，原因：\n%2")
                              .arg(type)
                              .arg(errorReasons.join("\n"));

            qWarning() << fullMsg;

        }
}

//测宽
void MainWindow::onWidthIP_DataReceived(const QByteArray &packet) {
    QString displayText;

    //qDebug()<<"宽度数据";
    //获取帧长度
    int totalPacketLen = (static_cast<uint8_t>(packet[2]) << 8) | static_cast<uint8_t>(packet[3]);
    //获取编码器值
    unsigned int EncoderCount = (static_cast<uint8_t>(packet[4]) << 8) | static_cast<uint8_t>(packet[5]);
    if(m_currentMode == CalcMode::EdgePoints)   //遮光低点高点算法
    {
        // 1 获取遮光低点和高点
        int low = 0,high = 0;//遮光低点，高点
        if (totalPacketLen == 0x0A) // 跳扫模式 (固定回复10字节，数据位占6字节)
        {
            low = (static_cast<uint8_t>(packet[6]) << 8) | static_cast<uint8_t>(packet[7]);
            high = (static_cast<uint8_t>(packet[8]) << 8) | static_cast<uint8_t>(packet[9]);
            displayText = QString("跳扫模式 - 编码器值:%1 低点:%2 高点:%3").arg(EncoderCount).arg(low).arg(high);
        }
        else // 全扫模式
        {
            QVector<uint8_t> status;
            for (int i = 6; i < packet.size(); ++i) { // 从灯珠状态位开始
                status.append(static_cast<uint8_t>(packet[i]));
            }
            //遮光低点高点计算--看实际算法需求是否需要
            int lowPoint = -1;  // 记录第一个被遮挡的索引
            int highPoint = 0; // 记录最后一个被遮挡的索引
            for (uint i = 0; i < LED_TOTAL; ++i) {
                bool blocked = (status[i/8] >> (i%8)) & 0x01;
                WidthLeds[i].isBlocked = blocked;
                if (blocked)//该灯珠遮光
                {
                    // 计算低点：记录第一次发现遮挡的位置
                    if (lowPoint == -1) lowPoint = i;
                    // 计算高点：不断更新，最后一次更新即为最高点
                    highPoint = i;
                }
            }

            if(lowPoint == -1)//遮光低点
            {
                low = 0;
            }
            high = highPoint;
            displayText = QString("全扫模式 - 测高 - 编码器值:%1 低点:%2 高点:%3").arg(EncoderCount).arg(low).arg(high);
            ui->lblData1->setText(displayText);
        }
        // 2 使用遮光低点和高点
        float startLamp = low - startWidth + 0.5;
        float endLamp = high - startWidth - 0.5;
        double numberOfLampsCovered = (endLamp > startLamp) ? (static_cast<double>(endLamp - startLamp + 1.0)) : 0.0;
        double width = numberOfLampsCovered * gratingSpacing;

        double now = QDateTime::currentMSecsSinceEpoch() / 1000.0;
        QMutexLocker locker(&profileMutex);
        if (width > 0) {
            if (!widthObjectDetected) {
                widthObjectDetected = true;
                widthObjectStartTime = now;
                widthProfile.clear();
                qDebug() << "宽度传感器检测到物体进入。";
            }
            MeasurementPoint p;
            p.timestamp = now - widthObjectStartTime;
            p.pulseCount = EncoderCount;
            //qDebug()<<"宽度当前脉冲数"<<pulseCount;
            //LOG_INFO("宽度当前脉冲数"+QString::number(pulseCount));
            p.value = width;
            p.startPoint = low - startWidth + 0.5;
            p.endPoint = high - startWidth - 0.5;
            widthProfile.append(p);

            ObjectInfo::RawSensorData rawData;
            rawData.timestamp = p.timestamp;
            rawData.startPoint = low;
            rawData.endPoint = high;
            rawData.value = width;
            rawData.pulseCount = p.pulseCount;
            lastMeasuredObject.widthRawData.append(rawData);
        } else if (widthObjectDetected) {
            widthObjectDetected = false;
            qDebug() << "宽度传感器检测到物体离开。轮廓点数:" << widthProfile.size();
            checkAndProcessObjectCompletion();
        }

    }
    else if(m_currentMode == CalcMode::FullPoints)  //全点还原算法
    {
        if (totalPacketLen == 0x0A) return; // 跳扫模式直接退出 跳扫模式不支持该算法
        // 全点还原算法
        QVector<uint8_t> status;
        for (int i = 6; i < packet.size(); ++i) {
            status.append(static_cast<uint8_t>(packet[i]));
        }

        QVector<int> currentBlocked;
        int lowPoint = -1;
        int highPoint = -1;

        for (uint i = 0; i < LED_TOTAL; ++i) {
            bool blocked = (status[i/8] >> (i%8)) & 0x01;
            WidthLeds[i].isBlocked = blocked;
            if (blocked) {
                currentBlocked.append(i);
                if (lowPoint == -1) lowPoint = i;
                highPoint = i;
            }
        }

        if (!currentBlocked.isEmpty()) {
            double now = QDateTime::currentMSecsSinceEpoch() / 1000.0;
            QMutexLocker locker(&profileMutex);

            if (!widthObjectDetected) {
                widthObjectDetected = true;
                widthObjectStartTime = now;
                widthProfile.clear();
                qDebug() << "宽度传感器检测到物体进入。";
            }

            MeasurementPoint p;
            p.timestamp = now - widthObjectStartTime;
            p.pulseCount = EncoderCount;
            p.blockedIndices = currentBlocked; // 保存所有点
            p.startPoint = lowPoint;
            p.endPoint = highPoint;
            p.value = (highPoint - lowPoint + 1) * gratingSpacing; // 维持一个基准宽度

            widthProfile.append(p);
        } else if (widthObjectDetected) {
            widthObjectDetected = false;
            qDebug() << "宽度传感器检测到物体离开。轮廓点数:" << widthProfile.size();
            checkAndProcessObjectCompletion();
        }
    }
}

//测高
void MainWindow::onHeightIP_DataReceived(const QByteArray &packet)
{
    QString displayText;
    //qDebug()<<"高度数据";
    //获取帧长度
    int totalPacketLen = (static_cast<uint8_t>(packet[2]) << 8) | static_cast<uint8_t>(packet[3]);
    //获取编码器值
    unsigned int EncoderCount = (static_cast<uint8_t>(packet[4]) << 8) | static_cast<uint8_t>(packet[5]);
    if(m_currentMode == CalcMode::EdgePoints)   //遮光低点高点算法
    {
        // 1 获取遮光低点和高点
        int low = 0,high = 0;//遮光低点，高点
        if (totalPacketLen == 0x0A) // 跳扫模式 (固定回复10字节，数据位占6字节)
        {
            low = (static_cast<uint8_t>(packet[6]) << 8) | static_cast<uint8_t>(packet[7]);
            high = (static_cast<uint8_t>(packet[8]) << 8) | static_cast<uint8_t>(packet[9]);
            displayText = QString("跳扫模式 - 编码器值:%1 低点:%2 高点:%3").arg(EncoderCount).arg(low).arg(high);
        }
        else // 全扫模式
        {
            QVector<uint8_t> status;
            for (int i = 6; i < packet.size(); ++i) { // 从灯珠状态位开始
                status.append(static_cast<uint8_t>(packet[i]));
            }
            //遮光低点高点计算--看实际算法需求是否需要
            int lowPoint = -1;  // 记录第一个被遮挡的索引
            int highPoint = 0; // 记录最后一个被遮挡的索引
            for (uint i = 0; i < LED_TOTAL; ++i) {
                bool blocked = (status[i/8] >> (i%8)) & 0x01;
                HeightLeds[i].isBlocked = blocked;
                if (blocked)//该灯珠遮光
                {
                    // 计算低点：记录第一次发现遮挡的位置
                    if (lowPoint == -1) lowPoint = i;
                    // 计算高点：不断更新，最后一次更新即为最高点
                    highPoint = i;
                }
            }

            if(lowPoint == -1)//遮光低点
            {
                low = 0;
            }
            high = highPoint;
            displayText = QString("全扫模式 - 测高 - 编码器值:%1 低点:%2 高点:%3").arg(EncoderCount).arg(low).arg(high);
            ui->lblData2->setText(displayText);
        }
        // 2 使用遮光低点和高点
        float startLamp = low;
        float endLamp = (high - 0.5);
        double numberOfLampsCovered = (endLamp > startLamp) ? (static_cast<double>(endLamp - startLamp) + 1.0) : 0.0;
        double height = numberOfLampsCovered * gratingSpacing;
        if(height > 0) height += fixheight;
        double now = QDateTime::currentMSecsSinceEpoch() / 1000.0;
        QMutexLocker locker(&profileMutex);
        if (height > 0) {
            if (!heightObjectDetected) {
                heightObjectDetected = true;
                heightObjectStartTime = now;
                heightProfile.clear();
                qDebug() << "高度传感器检测到物体进入。";
            }
            MeasurementPoint p;
            p.timestamp = now - heightObjectStartTime;
            p.pulseCount = EncoderCount;
            //qDebug()<<"高度当前脉冲数"<<pulseCount;
            p.value = height;
            p.startPoint = low;
            p.endPoint = high;
            heightProfile.append(p);

            ObjectInfo::RawSensorData rawData;
            rawData.timestamp = p.timestamp;
            rawData.startPoint = low;
            rawData.endPoint = high - 0.5;
            rawData.value = height;
            rawData.pulseCount = p.pulseCount;
            lastMeasuredObject.heightRawData.append(rawData);
        } else if (heightObjectDetected) {
            heightObjectDetected = false;
            qDebug() << "高度传感器检测到物体离开。轮廓点数:" << heightProfile.size();
            checkAndProcessObjectCompletion();
        }
    }
    else if(m_currentMode == CalcMode::FullPoints)  //全点还原算法
    {
        if (totalPacketLen == 0x0A) return; // 跳扫模式直接退出 跳扫模式不支持该算法
        // 全点还原算法
        QVector<uint8_t> status;
        for (int i = 6; i < packet.size(); ++i) {
            status.append(static_cast<uint8_t>(packet[i]));
        }

        QVector<int> currentBlocked;
        int lowPoint = -1;
        int highPoint = -1;

        for (uint i = 0; i < LED_TOTAL; ++i) {
            bool blocked = (status[i/8] >> (i%8)) & 0x01;
            HeightLeds[i].isBlocked = blocked;
            if (blocked) {
                currentBlocked.append(i);
                if (lowPoint == -1) lowPoint = i;
                highPoint = i;
            }
        }

        if (!currentBlocked.isEmpty()) {
            double now = QDateTime::currentMSecsSinceEpoch() / 1000.0;
            QMutexLocker locker(&profileMutex);

            if (!heightObjectDetected) {
                heightObjectDetected = true;
                heightObjectStartTime = now;
                heightProfile.clear();
                qDebug() << "高度传感器检测到物体进入。";
            }
            MeasurementPoint p;
            p.timestamp = now - heightObjectStartTime;
            p.pulseCount = EncoderCount;
            p.blockedIndices = currentBlocked; // 核心：保存所有点
            p.startPoint = lowPoint;
            p.endPoint = highPoint;
            p.value = (highPoint - lowPoint + 1) * gratingSpacing; // 维持一个基准宽度

            heightProfile.append(p);
        } else if (heightObjectDetected) {
            heightObjectDetected = false;
            qDebug() << "高度传感器检测到物体离开。轮廓点数:" << heightProfile.size();
            checkAndProcessObjectCompletion();
        }
    }
}

void MainWindow::onWidthIP_ShieldedParsed(const QByteArray &packet)
{
    qDebug()<<"宽度的屏蔽的灯珠状态为:"<<packet;
    unsigned int len = ((uint8_t)packet[2] << 8) | (uint8_t)packet[3];
    if(len == 68)
    {
        const uint8_t* ptr = (const uint8_t*)packet.data() + 4; // 跳过帧头和长度位
        for (uint i = 0; i < LED_TOTAL; ++i) {
            // 按照协议位图逻辑解析：第 i 个灯珠在第 i/8 字节的 i%8 位
            bool shielded = (ptr[i / 8] >> (i % 8)) & 0x01;
            WidthLeds[i].isShielded = shielded;
        }
        qDebug() << "硬件屏蔽状态已同步";
    }
    else
    {
        //TODO:数据帧长度错误
    }

}

void MainWindow::onHeightIP_ShieldedParsed(const QByteArray &packet)
{
    qDebug()<<"高度的屏蔽的灯珠状态为:"<<packet;
    unsigned int len = ((uint8_t)packet[2] << 8) | (uint8_t)packet[3];
    if(len == 68)
    {
        const uint8_t* ptr = (const uint8_t*)packet.data() + 4; // 跳过帧头和长度位
        for (uint i = 0; i < LED_TOTAL; ++i) {
            // 按照协议位图逻辑解析：第 i 个灯珠在第 i/8 字节的 i%8 位
            bool shielded = (ptr[i / 8] >> (i % 8)) & 0x01;
            HeightLeds[i].isShielded = shielded;
        }
        qDebug() << "硬件屏蔽状态已同步";
    }
    else
    {
        //TODO:数据帧长度错误
    }
}

void MainWindow::updateStatus(const QString &ip, bool connected) {
    QString state;
    if(connected)
    {
        state = "● 在线" ;
        if (ip == "192.168.10.3")
        {
            ui->lblStatus1->setText(state);
            QTimer::singleShot(500,this,[this]{

                // 遍历所有灯珠，设置对应屏蔽位
                for (uint i = 0; i < LED_TOTAL; ++i) {
                    // 仅处理：处于遮光状态 且 未被屏蔽的灯珠
                    if (WidthLeds[i].isBlocked && !WidthLeds[i].isShielded) {
                        // 更新本地灯珠状态
                        WidthLeds[i].isShielded = true;
                    }
                }
                qDebug() << "测量宽度的 已批量设置遮光状态灯珠为屏蔽状态";
                emit SetWidthRasterParameter(WidthLeds);
                //TODO:判断屏蔽的灯珠是否超过设置的数量，并且是否有连续的三个屏蔽的灯珠数量，若有则，实际应用中需要报警提示，当前硬件继续使用误差较大
                checkShieldingStatus(WidthLeds, LED_TOTAL, "宽度测量");
            });
        }
        else
        {
            ui->lblStatus2->setText(state);
            QTimer::singleShot(500,this,[this]{
                // 遍历所有灯珠，设置对应屏蔽位
                for (uint i = 0; i < LED_TOTAL; ++i) {
                    // 仅处理：处于遮光状态 且 未被屏蔽的灯珠
                    if (HeightLeds[i].isBlocked && !HeightLeds[i].isShielded) {
                        // 更新本地灯珠状态
                        HeightLeds[i].isShielded = true;
                    }
                }
                qDebug() << "测量高度的 已批量设置遮光状态灯珠为屏蔽状态" ;
                emit SetHeightRasterParameter(HeightLeds);
                //TODO:判断屏蔽的灯珠是否超过设置的数量，并且是否有连续的三个屏蔽的灯珠数量，若有则，实际应用中需要报警提示，当前硬件继续使用误差较大
                // 调用报警自检
                checkShieldingStatus(HeightLeds, LED_TOTAL, "高度测量");

            });
        }
        //TODO:此处应该需要等两个光栅都连接成功后再发送这个信号，此处会发送两次，实际移植的时候应该放到两个光栅都连接成功后再发送
        emit GetRasterParameter();

    }
    else
    {
        state = "○ 离线" ;
        if (ip == "192.168.10.3")
        {
            ui->lblStatus1->setText(state);
        }
        else
        {
            ui->lblStatus2->setText(state);
        }
    }

}

void MainWindow::on_btnSend1_clicked() {
    if(worker1) worker1->sendRawCommand(QByteArray::fromHex("01A10004"));
}

void MainWindow::on_btnSend2_clicked() {
    if(worker2) worker2->sendRawCommand(QByteArray::fromHex("01A10004"));
}

void MainWindow::handVolumeInfo(float volume, float length, float width, float height, QImage image, QImage csImage)
{
    //数据
    QString ResultTextStr = QString("测量结果如下: 长:%1 mm 宽:%2 mm 高:%3 mm").arg(length).arg(width).arg(height);
    qDebug()<<ResultTextStr;
    ui->label_2->setText(ResultTextStr);
    //照片
    if(!image.isNull())
    {
        // 3. 计算最大可显示的正方形尺寸
        int maxSize = qMin(ui->labWidth->width(), ui->labWidth->height());

        // 4. 按比例缩小图片，保持1:1比例
        QImage scaledImage = image.scaled(
            maxSize, maxSize,        // 缩放到400×400
            Qt::KeepAspectRatio,     // 保持宽高比
            Qt::SmoothTransformation // 平滑缩放，画质更好
        );

        // 5. 将图片设置到Label
        ui->labWidth->setPixmap(QPixmap::fromImage(scaledImage));

        // 6. 设置Label的对齐方式为居中，确保图片在Label中居中显示
        ui->labWidth->setAlignment(Qt::AlignCenter);
    }
    if(!csImage.isNull())
    {
        // 3. 计算最大可显示的正方形尺寸（受限于Label的高度400）
        int maxSize = qMin(ui->labHeight->width(), ui->labHeight->height()); // 这里会得到400

        // 4. 按比例缩小图片，保持1:1比例
        QImage scaledImage = csImage.scaled(
            maxSize, maxSize,        // 缩放到400×400
            Qt::KeepAspectRatio,     // 保持宽高比
            Qt::SmoothTransformation // 平滑缩放，画质更好
        );

        // 5. 将图片设置到Label
        ui->labHeight->setPixmap(QPixmap::fromImage(scaledImage));

        // 6. 设置Label的对齐方式为居中，确保图片在Label中居中显示
        ui->labHeight->setAlignment(Qt::AlignCenter);
    }
}

void MainWindow::checkAndProcessObjectCompletion()
{
    if (!widthProfile.isEmpty() && !heightProfile.isEmpty() && !widthObjectDetected && !heightObjectDetected) {
        qDebug() << "两个轮廓均已完成，开始最终处理。";
        processFinalData();
        widthProfile.clear();
        heightProfile.clear();
    }
}

void MainWindow::processFinalData()
{
    //根据测宽的数据直接计算俯视图的尺寸,然后再计算高
    qDebug() << "计算尺寸中...";
    VolumeData TempVolumeValue;
    calculateWidthFromProfile(&TempVolumeValue);
    calculateHeightFromProfile(&TempVolumeValue);
    //生成2D视图数据
    // 直接使用轮廓数据进行绘制
    drawTopView(widthProfile,&TempVolumeValue);
    drawSideView(heightProfile,&TempVolumeValue);
    TempVolumeValue.GetReady();
    if(TempVolumeValue.isAllready)
    {
        RastervolumeData(1,TempVolumeValue.length,TempVolumeValue.width,TempVolumeValue.height,TempVolumeValue.image1,TempVolumeValue.image2);
    }
    qDebug() << "测量完成，准备下一个物体。";
}

void MainWindow::calculateWidthFromProfile(MainWindow::VolumeData *TempVolumeValue)
{
    if (widthProfile.isEmpty()) {
        qWarning() << "无法计算宽度尺寸：宽度轮廓为空。";
        lastMeasuredObject.length = 0.0;
        lastMeasuredObject.width = 0.0;
        lastMeasuredObject.angleDeg = 0.0;
        return;
    }

    qDebug() << "开始仅使用宽度轮廓计算尺寸...";

    // 步骤 1: 滤波 (可选，如果 profile 是原始数据，最好进行中值滤波)
    // 尽管没有调用 filterProfile，但为了稳健性，我们假定使用原始或预滤波的 widthProfile。
    MeasurementProfile profileToUse = widthProfile; // 或者 filterProfile(widthProfile, 5);

    if (profileToUse.size() < 2) {
        qWarning() << "轮廓点过少，中止计算。";
        lastMeasuredObject.length = 0.0;
        lastMeasuredObject.width = 0.0;
        lastMeasuredObject.angleDeg = 0.0;
        return;
    }

    // 用于存储真实的俯视图轮廓 (包含凹陷) 的多边形
    QPolygonF actualProfile;
    // 步骤 2: 重建 2D 俯视图点云 (X-Z)
    QVector<QPointF> points2D;  // 用于 OBB 计算的点集
    qint32 startPulse = profileToUse.front().pulseCount;
    points2D.reserve(profileToUse.size() * 20);
    // 全点模式
    if (m_currentMode == CalcMode::FullPoints)
    {
        for (const auto& widthPoint : profileToUse) {
            double z = (widthPoint.pulseCount - startPulse) * distancePerPulse;
            for (int ledIndex : widthPoint.blockedIndices) {
                // 建议使用固定的 m_widthOffset，全点模式下不建议减去动态的 startPoint
                double x = (ledIndex - startWidth) * gratingSpacing;
                QPointF p(z, -x);
                points2D.append(p);      // 用于计算凸包/OBB
                actualProfile.append(p); // 用于 UI 显示
            }
        }
    }
    else    //高低点还原算法
    {
        // 1. 正向遍历：添加上边界
        for (const auto& widthPoint : profileToUse) {
            if (widthPoint.value <= 0) continue;
            double z = (widthPoint.pulseCount - startPulse) * distancePerPulse;
            double x_start = widthPoint.startPoint * gratingSpacing;
            actualProfile.append(QPointF(z, -x_start));

            // 生成点云用于 OBB 计算
            int lampCount = static_cast<int>(widthPoint.endPoint - widthPoint.startPoint) + 1;
            for (int i = 0; i <= lampCount * pointsPerLamp; ++i) {
                double x = x_start + (static_cast<double>(i) * gratingSpacing) / pointsPerLamp;
                points2D.append(QPointF(z, -x));
            }
        }

        // 2. 反向遍历：添加下边界（注意：此处必须在循环体外！）
        for (int i = profileToUse.size() - 1; i >= 0; --i) {
            const auto& widthPoint = profileToUse[i];
            if (widthPoint.value <= 0) continue;
            double z = (widthPoint.pulseCount - startPulse) * distancePerPulse;
            double x_end = widthPoint.endPoint * gratingSpacing;
            actualProfile.append(QPointF(z, -x_end));
        }
        qDebug() << "2D 俯视图重建完成，生成了" << points2D.size() << "个点。";
    }

    if (points2D.size() < 3) {
        qWarning() << "生成的 2D 点云数量不足。";
        lastMeasuredObject.length = 0.0;
        lastMeasuredObject.width = 0.0;
        lastMeasuredObject.angleDeg = 0.0;
        return;
    }
    // =================================================================
    // 存储真实的俯视图轮廓
    last_top_view_profile = actualProfile;
    // =================================================================
    // 步骤 3: 凸包计算
    QVector<QPointF> hull = computeConvexHull(points2D);

    if (hull.size() < 3) {
        qWarning() << "无法计算凸包。";
        return;
    }

    // 步骤 4: OBB 计算（Oriented Bounding Box）
    double minArea = std::numeric_limits<double>::max();
    QVector2D minEdgeDir, minPerpDir;
    double minWidth = 0, minHeight = 0;
    // OBB 矩形的顶点，用于 2D 视图绘制
    QVector<QPointF> minRect(4);

    for (int i = 0, j = hull.size() - 1; i < hull.size(); j = i, i++) {
        QVector2D edge(hull[i].x() - hull[j].x(), hull[i].y() - hull[j].y());
        double edgeLength = edge.length();
        if (edgeLength < 1e-6) continue;

        QVector2D edgeDir = edge.normalized();
        QVector2D perpDir(-edgeDir.y(), edgeDir.x()); // 垂直方向

        double minDot = std::numeric_limits<double>::max();
        double maxDot = std::numeric_limits<double>::min();
        double minPerp = std::numeric_limits<double>::max();
        double maxPerp = std::numeric_limits<double>::min();

        // 投影所有凸包点到两个方向轴上
        for (const QPointF& p : hull) {
            QVector2D v(p.x(), p.y());
            double dot = QVector2D::dotProduct(v, edgeDir);
            double perp = QVector2D::dotProduct(v, perpDir);
            minDot = qMin(minDot, dot);
            maxDot = qMax(maxDot, dot);
            minPerp = qMin(minPerp, perp);
            maxPerp = qMax(maxPerp, perp);
        }

        // 旋转矩形的两个边长
        double width = maxDot - minDot;
        double height = maxPerp - minPerp;
        double area = width * height;

        if (area < minArea) {
            minArea = area;
            minWidth = width;
            minHeight = height;
            minEdgeDir = edgeDir;
            minPerpDir = perpDir;

            // 记录最小面积矩形的四个顶点 (在 X-Z 平面上)
            minRect[0] = (edgeDir * minDot + perpDir * minPerp).toPointF();
            minRect[1] = (edgeDir * maxDot + perpDir * minPerp).toPointF();
            minRect[2] = (edgeDir * maxDot + perpDir * maxPerp).toPointF();
            minRect[3] = (edgeDir * minDot + perpDir * maxPerp).toPointF();
        }
    }
        M_obbRect = minRect;
    // 步骤 5: 确定最终尺寸（长和宽）
    // 假设 OBB 较长的一边为“长度”（Z轴方向），较短的一边为“宽度”（X轴方向）
    bool widthIsLong = minWidth > minHeight;
    lastMeasuredObject.length = widthIsLong ? minWidth : minHeight;
    lastMeasuredObject.width = widthIsLong ? minHeight : minWidth;

    // 确定角度（较长边与 X 轴的夹角）
    QVector2D longDir = widthIsLong ? minEdgeDir : minPerpDir;
    // atan2(y, x) 计算弧度，y 对应 Z，x 对应 X
    lastMeasuredObject.angleDeg = qRadiansToDegrees(std::atan2(longDir.y(), longDir.x()));

    // 步骤 6: 组装 OBB 顶点（仅用于 2D 视图显示）
    // 由于只使用 2D 数据，OBB 只需要 4 个顶点，但为了复用 3D 绘图逻辑，我们映射 4 个 2D 点到 8 个 3D 顶点的底面。
    // 这里我们只更新 M_obbVertices 的前四个点（底面 X-Z 投影）。
    // 假设 Y=0 的平面（皮带表面）
    M_obbVertices.resize(8);
    M_obbVertices[0] = QVector3D(minRect[0].x(), 0, minRect[0].y());
    M_obbVertices[1] = QVector3D(minRect[1].x(), 0, minRect[1].y());
    M_obbVertices[2] = QVector3D(minRect[2].x(), 0, minRect[2].y());
    M_obbVertices[3] = QVector3D(minRect[3].x(), 0, minRect[3].y());
    M_obbVertices[4] = M_obbVertices[0]; M_obbVertices[4].setY(lastMeasuredObject.height); // 假设高度为 0 或使用之前测得的高度
    M_obbVertices[5] = M_obbVertices[1]; M_obbVertices[5].setY(lastMeasuredObject.height);
    M_obbVertices[6] = M_obbVertices[2]; M_obbVertices[6].setY(lastMeasuredObject.height);
    M_obbVertices[7] = M_obbVertices[3]; M_obbVertices[7].setY(lastMeasuredObject.height);

    // 步骤 7: 更新 UI
    //labelLength->setText(QString("长度: %1 mm").arg(lastMeasuredObject.length, 0, 'f', 2));
    //labelWidth->setText(QString("宽度: %1 mm").arg(lastMeasuredObject.width, 0, 'f', 2));
    // 无法从宽度数据中准确得知高度，保留上次结果或设置为 N/A
    //labelHeight->setText(QString("高度: N/A"));
    //labelAngle->setText(QString("角度: %1 °").arg(lastMeasuredObject.angleDeg, 0, 'f', 2));
    TempVolumeValue->length =lastMeasuredObject.length;
    TempVolumeValue->width = lastMeasuredObject.width;
    qDebug() << "纯宽度尺寸计算完成: L=" << lastMeasuredObject.length << "W=" << lastMeasuredObject.width << "A=" << lastMeasuredObject.angleDeg;
}

void MainWindow::calculateHeightFromProfile(MainWindow::VolumeData *TempVolumeValue)
{
    qDebug() << "高度Profile点数:" << heightProfile.size();
    if (heightProfile.isEmpty()) {
        qWarning() << "无法计算高度尺寸：高度轮廓为空。";
        lastMeasuredObject.height = 0.0;
        return;
    }

    qDebug() << "开始计算高度轮廓...";
    MeasurementProfile profileToUse = heightProfile;

    if (profileToUse.size() < 2) {
        qWarning() << "高度轮廓点过少。";
        lastMeasuredObject.height = 0.0;
        return;
    }

    // 用于存储真实侧视图点云（用于显示）
    QPolygonF actualSideProfile;
    // 用于计算的点云集（Y-Z平面）
    QVector<QPointF> points2D_YZ;

    qint32 startPulse = profileToUse.front().pulseCount;

    points2D_YZ.reserve(profileToUse.size() * 20);
    for (const auto& heightPoint : profileToUse)
    {
        if (heightPoint.value <= 0) continue;

        // Z 坐标：传送带行走距离
        double z = (heightPoint.pulseCount - startPulse) * distancePerPulse;

        if (m_currentMode == CalcMode::FullPoints && !heightPoint.blockedIndices.isEmpty())
        {
            // ==========================================
            // 【全点还原算法】还原侧面每一个物理遮挡点
            // ==========================================
            for (int ledIndex : heightPoint.blockedIndices) {
                // Y 坐标：灯珠索引 * 间距
                // 注意：这里用 -y 是为了符合绘图坐标系（Y轴向上为负方向）
                double y = (ledIndex) * gratingSpacing;
                QPointF p(z, y);

                points2D_YZ.append(p);
                actualSideProfile.append(p);
            }
        }
        else
        {
            // ==========================================
            // 【边缘还原算法】高低点填充逻辑
            // ==========================================
            double y_start = (heightPoint.startPoint) * gratingSpacing;
            int lampCount = static_cast<int>(heightPoint.endPoint - heightPoint.startPoint) + 1;
            int totalPointsInSlice = lampCount * pointsPerLamp;

            for (int i = 0; i <= totalPointsInSlice; ++i) {
                double y = y_start + (static_cast<double>(i) * gratingSpacing) / pointsPerLamp;
                QPointF p(z, y);

                points2D_YZ.append(p);
                actualSideProfile.append(p);
            }
        }
    }

    // 更新全局变量，用于 UI 侧视图散点绘制
    last_side_view_profile = actualSideProfile;

    if (points2D_YZ.isEmpty()) {
        lastMeasuredObject.height = 0.0;
        return;
    }

    // ==========================================
    // 确定最终高度：取所有还原点中 Y 方向的最大值
    // ==========================================
    double maxHeight = 0;
    for (const auto& p : points2D_YZ) {
        // 因为存储时用了 -y，这里取绝对值即为物理高度
        maxHeight = qMax(maxHeight, qAbs(p.y()));
    }

    lastMeasuredObject.height = maxHeight;
    TempVolumeValue->height = maxHeight;

    qDebug() << "高度计算完成:" << maxHeight << "mm，生成点数:" << actualSideProfile.size();

    if (!actualSideProfile.isEmpty()) {
        qDebug() << "第一个点坐标:" << actualSideProfile.first();
    }
}

void MainWindow::drawTopView(const MeasurementProfile &profile, MainWindow::VolumeData *TempVolumeValue)
{
    // 1. 基础校验
    if (last_top_view_profile.isEmpty() || M_obbRect.size() < 4) {
        return;
    }

    // 2. 准备画布
    int W = 800, H = 800;
    QImage image(W, H, QImage::Format_ARGB32);
    image.fill(QColor(15, 15, 30)); // 深色背景
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    // 3. 计算所有图形的总边界 (Point Cloud + OBB)
    QPolygonF boundsPoly;
    boundsPoly.append(last_top_view_profile); // 加入轮廓点
    boundsPoly.append(M_obbRect);             // 加入OBB点
    QRectF dataBounds = boundsPoly.boundingRect();

    // 4. 获取变换参数
    // 参数: 边界, 宽, 高, padding=50, flipX=true (修正你的镜像问题), flipY=false
    ViewContext ctx = computeFitTransform(dataBounds, W, H, 50.0, true, false);

    // ================== 第一层：绘制几何图形 (应用变换矩阵) ==================
    painter.save();
    painter.setTransform(ctx.matrix); // 应用 世界->屏幕 变换

    // 绘制轮廓点云
    if (m_currentMode == CalcMode::FullPoints) {
        // 全点模式：画点
        // 注意：线宽需要除以 scale，否则放大后点会变得巨非大
        painter.setPen(QPen(QColor(0, 255, 0, 150), 2.0 / ctx.scale));
        painter.drawPoints(last_top_view_profile);
    } else {
        // 边缘模式：画填充多边形
        painter.setPen(QPen(Qt::green, 2.0 / ctx.scale));
        painter.setBrush(QColor(0, 255, 0, 50));
        painter.drawPolygon(last_top_view_profile);
    }

    // 绘制 OBB 矩形框
    QPen obbPen(Qt::cyan, 2.0 / ctx.scale, Qt::DashLine);
    painter.setPen(obbPen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPolygon(M_obbRect);

    painter.restore(); // 恢复到无变换状态 (单位矩阵)
    // ======================================================================

    // ================== 第二层：绘制标尺 (使用屏幕坐标) ====================
    // 直接将物理坐标点映射到屏幕坐标点
    QPolygonF screenObb = ctx.matrix.map(QPolygonF(M_obbRect));

    if (screenObb.size() == 4) {
        // OBB 的 4 个屏幕顶点
        QPointF sp0 = screenObb[0];
        QPointF sp1 = screenObb[1];
        QPointF sp2 = screenObb[2];
        QPointF sp3 = screenObb[3];

        // 计算物理边长 (必须用原始 M_obbRect 计算，保证数值是真实的mm)
        double len01 = QVector2D(M_obbRect[0] - M_obbRect[1]).length();
        double len12 = QVector2D(M_obbRect[1] - M_obbRect[2]).length();

        // 简单逻辑：绘制两条邻边的标尺
        // drawSmartRuler 会自动处理文字旋转，不用担心它是长边还是宽边
        drawSmartRuler(painter, sp0, sp1, len01, Qt::cyan);
        drawSmartRuler(painter, sp1, sp2, len12, Qt::yellow);
    }

    // 5. 保存结果
    TempVolumeValue->image1 = image;
}

void MainWindow::drawSideView(const MeasurementProfile &profile, MainWindow::VolumeData *TempVolumeValue)
{
    // 1. 基础校验
    if (profile.isEmpty() || last_side_view_profile.isEmpty()) {
        return;
    }

    // 2. 准备画布
    int W = 800, H = 800;
    QImage image(W, H, QImage::Format_ARGB32);
    image.fill(QColor(15, 15, 30));
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    // 3. 计算边界
    // 构造 QPolygonF 以使用 boundingRect()
    QPolygonF polyProfile(last_side_view_profile);
    QRectF dataBounds = polyProfile.boundingRect();

    // 【可选项】: 物体在画面中“落地”（不悬空），取消下面这行的注释。
    // 如果注释掉，画面会自动放大物体，只显示物体本身（不显示下方的空白高度）。
     dataBounds = dataBounds.united(QRectF(dataBounds.left(), 0, dataBounds.width(), 1.0));

    // 4. 获取变换参数
    // 参数: 边界, 宽, 高, padding=50, flipX=false, flipY=true
    ViewContext ctx = computeFitTransform(dataBounds, W, H, 50.0, false, true);

    // ================== 第一层：绘制几何图形 ==================
    painter.save();
    painter.setTransform(ctx.matrix);

    // 绘制地平线 (X轴) - 作为一个参考，还是画出来比较好，但离物体可能有距离
    //painter.setPen(QPen(QColor(100, 100, 100), 1.0 / ctx.scale)); // 灰色，不抢眼
    // 画得足够长
    double groundY = 0;
    painter.drawLine(QPointF(dataBounds.left() - 1000, groundY), QPointF(dataBounds.right() + 1000, groundY));

    // 绘制侧视图轮廓
    if (m_currentMode == CalcMode::FullPoints) {
        painter.setPen(QPen(Qt::blue, 2.0 / ctx.scale));
        painter.drawPoints(last_side_view_profile);
    } else {
        painter.setPen(QPen(Qt::blue, 2.0 / ctx.scale));
        painter.setBrush(QColor(0, 0, 255, 50));
        painter.drawPolygon(polyProfile);
    }

    // --- 寻找最高点和最低点 ---
    QPointF maxPoint(0,0);
    double maxY = -999999.0;
    double minY = 999999.0; // 新增：寻找物体的最低点

    for (const auto& p : last_side_view_profile) {
        if (p.y() > maxY) {
            maxY = p.y();
            maxPoint = p;
        }
        if (p.y() < minY) {
            minY = p.y();
        }
    }

    // 防止数据异常导致的错乱
    if (minY > maxY) minY = 0;

    // 画虚线：现在只从“最高点”画到“物体最低点”，不再画穿到地平线
    QPen dashPen(Qt::white, 1.0 / ctx.scale, Qt::DashLine);
    painter.setPen(dashPen);
    painter.drawLine(maxPoint, QPointF(maxPoint.x(), minY));

    painter.restore();
    // ========================================================

    // ================== 第二层：绘制标尺 =====================
    // 逻辑坐标：标尺从物体的最低点(minY) 画到 最高点(maxY)
    // 这样标尺就会紧贴着物体，哪怕物体是悬空的
    QPointF pBottomLogic(maxPoint.x(), minY);
    QPointF pTopLogic = maxPoint;

    QPointF pBottomScreen = ctx.matrix.map(pBottomLogic);
    QPointF pTopScreen = ctx.matrix.map(pTopLogic);

    // 计算物体实际高度 (Max - Min)
    double objectHeight = maxY - minY;

    // 绘制高度标尺 (使用红色)
    drawSmartRuler(painter, pBottomScreen, pTopScreen, objectHeight, QColor(255, 100, 100));

    // 5. 保存结果
    TempVolumeValue->image2 = image;
}

qreal MainWindow::cross_product(const QPointF &o, const QPointF &a, const QPointF &b) {
    return (a.x() - o.x()) * (b.y() - o.y()) - (a.y() - o.y()) * (b.x() - o.x());
}
QVector<QPointF> MainWindow::computeConvexHull(QVector<QPointF> &points)
{
    if (points.size() <= 3) return points;
    std::sort(points.begin(), points.end(), [](const QPointF &a, const QPointF &b) {
        return a.x() < b.x() || (a.x() == b.x() && a.y() < b.y());
    });
    QVector<QPointF> lower_hull;
    for (const auto &p : points) {
        while (lower_hull.size() >= 2 && cross_product(lower_hull[lower_hull.size()-2], lower_hull.back(), p) <= 0) {
            lower_hull.pop_back();
        }
        lower_hull.push_back(p);
    }
    QVector<QPointF> upper_hull;
    for (int i = points.size() - 1; i >= 0; --i) {
        const auto &p = points[i];
        while (upper_hull.size() >= 2 && cross_product(upper_hull[upper_hull.size()-2], upper_hull.back(), p) <= 0) {
            upper_hull.pop_back();
        }
        upper_hull.push_back(p);
    }
    lower_hull.pop_back();
    upper_hull.pop_back();
    return lower_hull + upper_hull;
}

void MainWindow::drawRulerOnEdge(QPainter &painter, const QPointF &p1, const QPointF &p2, double dimension, double scale, bool mirrorText, bool isBottomEdge)
{
    // 过滤无效尺寸
    if (dimension < 1e-6) return;

    // ========== 修复坐标系计算：先保存全局状态，重置计算角度 ==========
    painter.save(); // 第一层save：保存全局变换
    QTransform globalTransform = painter.transform();
    painter.resetTransform(); // 重置为原始坐标系计算角度

    // 基于原始坐标系计算边的角度（避免全局scale干扰）
    QVector2D edgeVec(p2 - p1);
    double edgeAngle = qRadiansToDegrees(std::atan2(edgeVec.y(), edgeVec.x()));

    painter.setTransform(globalTransform); // 恢复全局变换

    // ========== 保存全局状态，用于绘制 ==========
    painter.save(); // 第二层save：绘制专用

    // 变换顺序（和你原始代码一致：先平移后旋转）
    painter.translate(p1);
    painter.rotate(edgeAngle);

    // --- 1. 标尺样式（完全和你原始代码一致） ---
    const double offset = 10.0 / scale;
    const double textMargin = 5.0 / scale;
    const QColor rulerColor = QColor(40, 220, 255);
    painter.setPen(QPen(rulerColor, 1.0 / scale));
    QFont font("Arial");
    font.setPointSizeF(20.0 / scale);
    painter.setFont(font);

    // --- 2. 刻度间隔（和你原始代码一致） ---
    double majorInterval = 50.0;
    if (dimension > 500) majorInterval = 100.0;
    else if (dimension < 100) majorInterval = 10.0;
    else if (dimension < 200) majorInterval = 20.0;

    double minorInterval = 1.0;
    if (dimension > 50) minorInterval = 5.0;
    if (dimension > 200) minorInterval = 10.0;

    const double majorTickHeight = 8.0 / scale;
    const double minorTickHeight = 4.0 / scale;

    // --- 3. 绘制标尺线（和你原始代码一致） ---
    painter.drawLine(QPointF(0, 0), QPointF(0, offset));
    painter.drawLine(QPointF(dimension, 0), QPointF(dimension, offset));

    // --- 4. 绘制刻度线（仅画线，无文本，和你原始代码一致） ---
    for (double d = 0; d <= dimension + minorInterval/2; d += minorInterval) {
        if (d > dimension) continue;
        bool isMajorTick = (std::fmod(d, majorInterval) < 1e-6);

        if (isMajorTick) {
            painter.drawLine(QPointF(d, offset), QPointF(d, offset + majorTickHeight));
            // 无刻度文本（和你原始代码一致）
        } else {
            painter.drawLine(QPointF(d, offset), QPointF(d, offset + minorTickHeight));
        }
    }

    // --- 5. 绘制总尺寸标签（核心修复：仅底部翻转，其他完全还原你的逻辑） ---
    QString totalDimText = QString("%1 mm").arg((int)dimension);
    QRectF textBounds = painter.fontMetrics().boundingRect(totalDimText);
    painter.save(); // 第三层save：文本专用

    if (isBottomEdge) {
        // 仅底部文本垂直翻转（核心修复，坐标和你原始代码一致）
        painter.translate(dimension / 2.0, -textMargin); // 和你原始坐标一致
        painter.scale(1, -1); // 垂直翻转
        painter.drawText(QPointF(-textBounds.width()/2.0, 0), totalDimText); // 居中
    } else {
        // 非底部边：完全还原你原始代码的逻辑（无任何改动）
        if (mirrorText) {
            painter.translate(dimension / 2.0, 0);
            painter.scale(-1, 1);
            painter.drawText(QPointF(-textBounds.width()/2.0, -textMargin), totalDimText);
        } else {
            painter.drawText(QPointF(dimension / 2.0 - textBounds.width()/2.0, -textMargin), totalDimText);
        }
    }

    painter.restore(); // 第三层restore

    // ========== 恢复变换（严格匹配） ==========
    painter.restore(); // 第二层restore
    painter.restore(); // 第一层restore
}
