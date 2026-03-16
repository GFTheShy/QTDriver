#ifndef SINGLETON_H
#define SINGLETON_H
#include <QString>
#include <QDebug>
/*
 * 单例模式
*/
class AppConfig
{
public:
    static AppConfig& getInstance(){
        static AppConfig Instance;
        return Instance;
    };
    void setValue(QString key,QString value)
    {
        qDebug()<<"设置参数"<<key<< value;
    };
    void getValue(QString key,QString value)
    {
        qDebug()<<"得到参数"<<key<< value;
    };
private:
    AppConfig();
    ~AppConfig();
};

#endif // SINGLETON_H

/*调用示例：
    class AppConfig &Instance = AppConfig::getInstance();
    Instance.setValue("2","aa");
    Instance.getValue("3","bb");
 */
