#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QList>
#include <QLabel>
#include <QPoint>
#include <QRect>
#include <QTimer>
#include <QMouseEvent>
#include <QMenu>
#include <QAction>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QByteArray>

#include "weatherTool.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

struct Today
{
    QString province;   //省份
    QString city;       //城市
    QString type;       //天气类型
    QString tempature;  //温度
    QString fx;         //风向
    QString fl;         //风力
    QString humidity;   //湿度
    QString date;       //日期

    QString quality;    //没有   空气质量  设置默认值
    QString sunrise;    //      日出时间 设置默认值
    QString sunset;     //      日落时间 设置默认值
};

//周几
//日期
//风力
//天气状况
//天气图标
//最高气温
//最低气温
struct Forecast
{
    QString week;   //周几
    QString date;   //日期
    QString power;  //风力
    QString type;   //天气状况
    QString high;   //最高气温
    QString low;    //最低气温
};

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

protected:
    bool eventFilter(QObject* watched, QEvent* event);
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void contexMenuEvent(QContextMenuEvent* menuEvent);

private slots:
    void on_searchBt_clicked();

    void on_refreshBt_clicked();

    void replayFinished(QNetworkReply* reply);  //2、获取响应的信息
    void slot_exitApp();

private:
    void getWeatherInfo(QNetworkAccessManager* manager, bool isToday); //1、网络请求数据
    void parseJson(QByteArray bytes);   //3、解析json数据
    void parseToday(QByteArray bytes);
    void setLabelConnect();
    void setTodayConnect();
    void paintSunRiseSet();
    void paintCurve();
    void callKeyBoard();

private:
    Ui::Widget *ui;

    QPoint mPos;

    QList<QLabel*> forecast_week_list;      //星期数 label(第一行)
    QList<QLabel*> forecast_date_list;      //日期数 （第二行
    QList<QLabel*> forecast_aqi_list;       //质量   （第三行
    QList<QLabel*> forecast_type_list;      //天气类型  （第四行
    QList<QLabel*> forecast_typeIco_list;   //天气图标  （第五行
    QList<QLabel*> forecast_high_list;      //最高温     （第六行
    QList<QLabel*> forecast_low_list;       //最低温    （第八行

    Today today;
    Forecast forecast[6];

    static const QPoint sun[2];
    static const QRect sunRiseSet[2];
    static const QRect rect[2];

    QTimer* sunTimer;

    QNetworkAccessManager* manager;
    QNetworkReply* reply;
    QString url;
    QString key;
    QString extensions;
    QString city;
    QString cityTmp;
    WeatherTool tool;

    QMenu* m_pMenu;
    QAction* m_pExitAct;
};
#endif // WIDGET_H
