#ifndef FACTORY_H
#define FACTORY_H
#include <QGraphicsItem>
#include <QDebug>
/*
工厂模式
*/
//产品类型
enum ShapeType{
    rectangle = 0,
    roune,
    triangle
};

//抽象产品
class Shape
{
public:
    //产品的通用功能或属性
    virtual QGraphicsItem* createItem() const = 0;//创建图形
    virtual void printShape() const = 0;//输出属性
};

//具体产品
class Rectangle : public Shape
{
public:
    //返回矩形
    QGraphicsItem* createItem() const
    {
        return new QGraphicsRectItem();
    }
    void printShape() const{
        qDebug()<<"返回矩形"<<endl;
    }
};

class Circle : public Shape
{
public:
    //返回圆
    QGraphicsItem* createItem() const
    {
        return new QGraphicsEllipseItem;
    }
    void printShape() const{
        qDebug()<<"返回圆"<<endl;
    }
};

class Triangle : public Shape
{
public:
    //返回三角形
    QGraphicsItem* createItem() const
    {
        return new QGraphicsPolygonItem();
    }
    void printShape() const{
        qDebug()<<"返回三角形"<<endl;
    }
};

//工厂
class ShapeFactory
{
public:
    Shape* CreateShape(ShapeType type){
        switch(type)
        {
            case ShapeType::rectangle:
                return new Rectangle();
            case ShapeType::roune:
                return new Circle();
            case ShapeType::triangle:
                return new Triangle();
        };
    };
};
#endif // FACTORY_H


/*调用示例：
    class ShapeFactory factory;//创建工厂
    Shape *shape;//创建抽象产品
    shape = factory.CreateShape(rectangle);//工厂生产具体产品
    shape->printShape();//使用产品
    shape = factory.CreateShape(roune);//工厂生产具体产品
    shape->printShape();//使用产品
    shape = factory.CreateShape(triangle);//工厂生产具体产品
    shape->printShape();//使用产品
*/
