#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>
#include "lightcurtainworker.h"
#include <QMutex>
#include <QDateTime>
#include <QMutexLocker>
#include <QVector2D>
#include <QtMath>
#include <QVector3D>
#include <QPainter>
#include <QFontMetrics>
#include <QTimer>
QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 辅助结构体：保存视图变换参数
struct ViewTransform {
    double scale;
    QPointF offset;
    QTransform matrix; // 世界 -> 屏幕 的变换矩阵
};

// 代表一个测量时间点 (时间戳和测量值)
struct MeasurementPoint {
    double timestamp;    // 保留用于调试/参考
    qint32 pulseCount;   // 测量瞬间的编码器脉冲数
    double value;
    float startPoint = 0;
    float endPoint = 0;
    QVector<int> blockedIndices; // 记录所有遮挡灯珠的索引 (e.g., {10, 11, 15, 16})
};

// 代表一个经过物体的完整轮廓 (宽度或高度)
using MeasurementProfile = QVector<MeasurementPoint>;
// 存储一个已测量物体的所有信息
struct ObjectInfo {
    double length;
    double width;
    double height;
    double angleDeg;
    QDateTime time;

    //原始传感器数据
    struct RawSensorData {
        double timestamp;    // 时间戳（相对于物体开始时间）
        float startPoint;    // 遮光低点
        float endPoint;      // 遮光高点
        double value;        // 计算值（宽度或高度）
        qint32 pulseCount;   // 测量瞬间的编码器脉冲数
    };

    QVector<RawSensorData> widthRawData;   // 宽度传感器原始数据
    QVector<RawSensorData> heightRawData;  // 高度传感器原始数据
};
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    //测量算法模式
    enum class CalcMode {
        EdgePoints, // 模式1：基于高低点
        FullPoints  // 模式2：基于全点云（仅全扫生效）
    };

    CalcMode m_currentMode = CalcMode::FullPoints;

private slots:
    void onWidthIP_DataReceived(const QByteArray &packet);//灯珠状态数据 功能码A1的数据包
    void onHeightIP_DataReceived(const QByteArray &packet);//灯珠状态数据 功能码A1的数据包
    void onWidthIP_ShieldedParsed(const QByteArray &packet);//屏蔽的灯珠数据 功能码A7的数据包
    void onHeightIP_ShieldedParsed(const QByteArray &packet);//屏蔽的灯珠数据 功能码A7的数据包

    void updateStatus(const QString &ip, bool connected);
    void on_btnSend1_clicked();
    void on_btnSend2_clicked();
    void handVolumeInfo(float volume,
          float length, float width, float height,
                        QImage image,QImage csImage);   //测量模块的回调函数的数据
signals:
    void SetWidthRasterParameter(const QVector<LedNode> &Leds);   //设置宽度光栅参数信号
    void SetHeightRasterParameter(const QVector<LedNode> &Leds);  //设置高度光栅参数信号
    void GetRasterParameter();                          //获取光栅参数信号
    void RastervolumeData(float volume, float length,
                            float width, float height,QImage image,QImage csImage); // 光栅的整体尺寸信息信号
private:
    Ui::MainWindow *ui;
    /*---已经有的无需移植到软件的参数---*/
    struct VolumeData
    {
        float volume;
        float length=0;
        float width=0;
        float height=0;
        QImage image1;
        QImage image2;
        bool isAllready=false;

        void GetReady(){
            if(length > 0 && width > 0 && height > 0 && !image1.isNull() && !image2.isNull())
            {
                isAllready = true;
            }
        }
    };
    int startWidth = 0;                                //屏蔽的灯珠从哪开始，得到的序号需要减去这个
    qreal gratingSpacing = 2;                         //光栅间距 (mm)
    QMutex profileMutex;
    bool widthObjectDetected = false;
    double widthObjectStartTime = 0.0;
    MeasurementProfile widthProfile;                    //测宽的数据缓冲区
    ObjectInfo lastMeasuredObject;                      //最近一次的测量的完整数据
    void checkAndProcessObjectCompletion();             //检查是否物品完全通过
    double fixheight = 0;                               //光栅第一个灯距离皮带的高度修补(mm)
    bool heightObjectDetected = false;
    double heightObjectStartTime = 0.0;
    MeasurementProfile heightProfile;                   //测高的数据缓冲区
    void processFinalData();                            //最终处理
    void calculateWidthFromProfile(VolumeData *TempVolumeValue);                   //计算宽度
    void calculateHeightFromProfile(VolumeData *TempVolumeValue);                  //计算高度
    void drawTopView(const MeasurementProfile& profile,VolumeData *TempVolumeValue);//生成俯视图
    void drawSideView(const MeasurementProfile& profile,VolumeData *TempVolumeValue);//生成侧视图
    double distancePerPulse = 0.1433787684;             //编码器单脉冲距离
    int pointsPerLamp = 1;                              //宽度每个灯生成的点，控制点云生成的密度
    QVector<QPointF> last_top_view_profile;             // 用于保存上一次计算的俯视图凸包
    QVector<QPointF> last_side_view_profile;            // 用于保存上一次计算的侧视图凸包
    qreal cross_product(const QPointF &o, const QPointF &a, const QPointF &b);
    QVector<QPointF> computeConvexHull(QVector<QPointF> &points);// 辅助函数，2d凸包计算
    QVector<QPointF> M_obbRect;                         // 用于保存 OBB 的4个顶点，以便绘制
    QVector<QVector3D> M_obbVertices;
    QVector<QPointF> last_side_hull;                    // 用于保存上一次计算的侧视图凸包
    void drawRulerOnEdge(QPainter &painter,
                         const QPointF& p1,
                         const QPointF& p2,
                         double dimension,
                         double scale,
                         bool mirrorText,
                         bool isBottomEdge);                 // 辅助函数，用于沿一条边绘制尺寸标尺
    /*---已经有的无需移植到软件的参数---*/


    /*---需要移植到软件的参数---*/
    QThread *thread1 = nullptr;// 必须将线程对象作为成员变量，保证其生命周期
    QThread *thread2 = nullptr;
    LightCurtainWorker *worker1 = nullptr;
    LightCurtainWorker *worker2 = nullptr;
    unsigned int LED_TOTAL = 448;//灯珠数量
    QVector<LedNode> WidthLeds; //测宽灯珠列表
    QVector<LedNode> HeightLeds;//测高灯珠列表
    void initDevice(const QString &ip, int port, int id);
    // --- 绘图辅助结构与函数 ---
    // 1. 视图变换结果包：包含缩放比例和变换矩阵
    struct ViewContext {
        double scale;       // 缩放比例 (像素/mm)
        QTransform matrix;  // 世界坐标 -> 屏幕坐标 的变换矩阵
        QRectF screenRect;  // 绘制区域的屏幕边界
    };

    // 2. 通用函数：计算最佳适配的视图变换矩阵
    ViewContext computeFitTransform(const QRectF& dataBounds, int imgWidth, int imgHeight, double padding, bool flipX, bool flipY);

    // 3. 通用函数：在两点之间绘制“智能标尺”（自动调整文字方向）
    // 参数 p1, p2 必须是屏幕坐标(像素)
    void drawSmartRuler(QPainter &painter, QPointF p1, QPointF p2, double actualValueMM, const QColor& color = Qt::cyan);

    // 检查屏蔽状态并报警
    void checkShieldingStatus(QVector<LedNode> &leds, unsigned int total, const QString &type);

    // --- 报警配置参数 ---
    const int SAFE_ZONE_START = 5;      // 忽略前5个灯珠（安装位）
    const int SAFE_ZONE_END = 5;        // 忽略最后5个灯珠（安装位）
    const int MAX_SHIELDED_LIMIT = 10;  // 有效区域内允许的最大屏蔽总数
    const int CONTINUOUS_THRESHOLD = 3; // 有效区域内允许的最大连续屏蔽数
    /*---需要移植到软件的参数---*/
};

#endif
