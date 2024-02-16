#ifndef WEATHERTOOL_H
#define WEATHERTOOL_H

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

#include <map>
#include <QFile>
#include <QCoreApplication>

class WeatherTool
{
public:
    WeatherTool()
    {
        QString filename = QCoreApplication::applicationDirPath();
        qDebug() << "当前父目录: " << filename;  //注意这里的目录是build目录
        QJsonParseError err;
        filename += "/citycode-2023.json";
        qDebug() << "拼接完的目录: " << filename;

        QFile file(filename);
        bool ret = file.open(QIODevice::ReadOnly | QIODevice::Text);
        if(ret)
        {   //文件打开成功才有接下来的步骤
            qDebug() << "文件打开成功, 正在读取...";

            QByteArray json = file.readAll();
            file.close();

            QJsonDocument jsonDoc = QJsonDocument::fromJson(json, &err);
            QJsonArray citys = jsonDoc.array();
            qDebug() << "读取到的json大小: " << citys.size();
            for (int i = 0; i < citys.size(); ++i)
            {
                QString code = citys.at(i).toObject().value("city_code").toString();
                QString city = citys.at(i).toObject().value("city_name").toString();
                if(code.size() > 0)
                {
                    m_mapCity2Code.insert(std::pair<QString, QString>(city, code));
                }
            }
            qDebug() << "json数据读取完毕";
        }
        else
        {
            qDebug() << "文件打开失败";
        }
    }

    QString operator[](const QString& city)
    {
        std::map<QString, QString>::iterator it = m_mapCity2Code.find(city);
        if(it == m_mapCity2Code.end())
        {
            //如果没找到，就循环遍历一个个查找
            for (auto iter = m_mapCity2Code.begin(); iter != m_mapCity2Code.end(); iter++)
            {
                if(iter->first.contains(city))  //包含就返回城市代号
                    return iter->second;
            }
        }
        if(it != m_mapCity2Code.end())
        {
            return it->second;
        }
        return "000000";  //都没找到，就返回中华人民共和国
    }

private:
    std::map<QString, QString> m_mapCity2Code;
};


#endif // WEATHERTOOL_H
