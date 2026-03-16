#ifndef BUILDERPATTERN_H
#define BUILDERPATTERN_H
#include <QString>
/*
 * 建造者模式
*/
//具体要生产的产品
class Computer
{
public:
    void setcpu(QString cpu){m_cpu = cpu;}
    void setmemory(QString memory){m_memory = memory;}
    void sethardDisk(QString hardDisk){m_hardDisk = hardDisk;}
    void setgraphicsCard(QString graphicsCard){m_graphicsCard = graphicsCard;}
private:
    QString m_cpu;
    QString m_memory;
    QString m_hardDisk;
    QString m_graphicsCard;

};
//抽象建造者
class ComputerBuilder
{
public:
    virtual void setCpu() = 0;
    virtual void setmemoryCpu() = 0;
    virtual void sethardDisk() = 0;
    virtual void setgraphicsCard() = 0;
    virtual Computer* getComper() = 0;
};
//具体建造者
class myComputerBuilder : public ComputerBuilder
{
public:
    myComputerBuilder(){
        m_computer = new Computer;
    };

    void setCpu()
    {
        m_computer->setcpu("cpu");
    };
    void setmemoryCpu()
    {
        m_computer->setmemory("memory");
    };
    void sethardDisk()
    {
        m_computer->sethardDisk("hardDisk");
    };

    void setgraphicsCard()
    {
        m_computer->setgraphicsCard("graphicsCard");
    };
    Computer* getComper()
    {
        return m_computer;
    };
private:
    Computer *m_computer;
};
//具体指挥者
class Director
{
public:
    Director(ComputerBuilder *computerBuilder=0){
        computer = computerBuilder;
    }
    void setComputerBuilder(ComputerBuilder *computerBuilder)
    {
        computer = computerBuilder;
    };
    Computer *buildComputer(){
        computer->setCpu();
        computer->sethardDisk();
        computer->setmemoryCpu();
        computer->setgraphicsCard();
        return computer->getComper();
    };
private:
    ComputerBuilder *computer;
};
#endif // BUILDERPATTERN_H

/*调用示例：
    class Director director(new myComputerBuilder());
    Computer *computer;
    computer = director.buildComputer();
 */

