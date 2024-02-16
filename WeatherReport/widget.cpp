#pragma warning (disable:4819)
#include "widget.h"
#include "ui_widget.h"

#include <QPainter>
#include <QDateTime>
#include <QMessageBox>

#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>

#define SPAN_INDEX 3  //温度曲线间隔指数
#define ORIGIN_SIZE 3 //温度曲线远点大小
#define TEMPERATURE_STARTING_COORDINATE 25  //高温平均值起始坐标

//日出日落底线
const QPoint Widget::sun[2] = {
    QPoint(20,75),
    QPoint(130,75)
};

//日出日落事件
const QRect Widget::sunRiseSet[2] = {
    QRect(0,80,50,20),
    QRect(100,80,50,20)
};

//日出日落圆弧
const QRect Widget::rect[2] = {
    QRect(25,25,100,100),   //虚线圆弧
    QRect(50,80,50,20)      //"日出日落"文本
};

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    this->setWindowFlag(Qt::FramelessWindowHint); //隐藏边框

    ui->cityLineEdit->setStyleSheet("QLineEdit{border: 1px solid gray; border-radius: 4px; background:rgb(85, 85, 127); color:rgb(255, 255, 255);} QLineEdit:hover{border-color:rgb(101, 255, 106); }");
    ui->refreshBt->setStyleSheet("QPushButton{border: 2px solid gray; border-radius: 4px;} QPushButton:hover{border-color:rgb(0, 255, 106); }");


    forecast_week_list << ui->week1Lb << ui->week2Lb << ui->week3Lb << ui->week4Lb;
    forecast_date_list << ui->date1Lb << ui->date2Lb << ui->date3Lb << ui->date4Lb;
    forecast_aqi_list << ui->quality1Lb << ui->quality2Lb << ui->quality3Lb << ui->quality4Lb;
    forecast_type_list << ui->type1Lb << ui->type2Lb << ui->type3Lb << ui->type4Lb;
    forecast_typeIco_list << ui->typeIco1Lb << ui->typeIco2Lb << ui->typeIco3Lb << ui->typeIco4Lb;
    forecast_high_list << ui->high1Lb << ui->high2Lb << ui->high3Lb << ui->high4Lb;
    forecast_low_list << ui->low1Lb << ui->low2Lb << ui->low3Lb << ui->low4Lb;

    //结构体初始化 当日详细数据
    today.province = "null";
    today.city = "null";
    today.type = u8" 无数据 ";
    today.tempature = "null";
    today.fx = u8" 无数据 ";
    today.fl = u8" 无数据 ";
    today.humidity = u8" 无数据 ";
    today.date = "0000-00-00";

    today.quality = u8"还行";
    today.sunrise = "00:00";
    today.sunset = "19:00";

    for (int i = 0; i < 4; ++i)
    {
        forecast[i].week = "星期0";          //周几
        forecast[i].date = "yyyy-MM-dd";    //日期
        forecast[i].power = "8";            //风力
        forecast[i].type = "undefined";     //天气状况
        forecast[i].high = " 高温 0.0°C";    //最高气温
        forecast[i].low = " 低温 0.0°C";     //最低气温
    }

    //右键菜单
    m_pMenu = new QMenu(this);
    m_pExitAct = new QAction;
    m_pExitAct->setText(u8" 退出 ");
    m_pExitAct->setIcon(QIcon(":/icons/close.ico"));
    m_pMenu->addAction(m_pExitAct);
    connect(m_pExitAct, SIGNAL(triggered(bool)), this, SLOT(slot_exitApp()));

    //dateLb和weekLb样式表设置
    for (int i = 0; i < 4; ++i)
    {
        forecast_date_list[i]->setStyleSheet("background-color: rgba(0, 255, 255, 100);");
        forecast_week_list[i]->setStyleSheet("background-color: rgba(0, 255, 255, 100);");
    }

    //请求天气API信息
    //高德天气API使用： jsonUrl = url + city_code + key
    //原本是https://  但是如果使用https会在传输时多一层加密，这层加密需要用到openssl去解析，但是qt程序本身不让用(想用需要自己加一个dll进来)
    //所以改成http,虽然网站会提示不安全，毕竟后面有密钥我还用非加密的，但是起码不用再麻烦的手动去装openssl就能正常访问了
    url = "http://restapi.amap.com/v3/weather/weatherInfo?city=";
    key = "&key=440d2191d8abbf736b3e24e18ec71488";
    extensions = "&extensions=all";
    //usage: jsonUrl = url + citycode + key + extensions  返回结果是包含今天在内的四天的天气信息

    city = u8"北京";
    cityTmp = city;
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replayFinished(QNetworkReply*)));
    getWeatherInfo(manager, false);

    //事件过滤
    ui->sunRiseSetLb->installEventFilter(this); //启用事件过滤器
    ui->curveLb->installEventFilter(this);
    ui->cityLineEdit->installEventFilter(this);
}

Widget::~Widget()
{
    delete ui;
}

//事件过滤
bool Widget::eventFilter(QObject* watched, QEvent* event)
{
    if(watched == ui->sunRiseSetLb && event->type() == QEvent::Paint)
    {
        paintSunRiseSet();
    }
    else if(watched == ui->curveLb && event->type() == QEvent::Paint)
    {
        paintCurve();
    }
    else if(watched == ui->cityLineEdit && event->type() == QEvent::MouseButtonPress)
    {
        callKeyBoard();  //调用软键盘
    }
    return QWidget::eventFilter(watched, event); //上交给父类继续处理
}

//窗口移动
void Widget::mouseMoveEvent(QMouseEvent* event)
{
    this->move(event->globalPos() - mPos);
}

void Widget::mousePressEvent(QMouseEvent* event)
{
    mPos = event->globalPos() - this->pos();
}

//右键菜单
void Widget::contexMenuEvent(QContextMenuEvent* menuEvent)
{
    m_pMenu->exec(QCursor::pos());
    menuEvent->accept();
}

//搜索按钮
void Widget::on_searchBt_clicked()
{
    cityTmp = city;
    city = ui->cityLineEdit->text();
    getWeatherInfo(manager, false);
}

//刷新按钮
void Widget::on_refreshBt_clicked()
{
    getWeatherInfo(manager, false);
    //ui->curveLb->update();   TODO:画线的先不调用
}

//获取响应的信息
void Widget::replayFinished(QNetworkReply* reply)
{
    QByteArray bytes = reply->readAll();

    qDebug() << "bytes size: " << bytes.size();
    if(bytes.size() == 0)
    {
        QMessageBox::warning(this, u8"错误", u8"天气:请求数据错误，请检查网络连接!", QMessageBox::Ok);
        return;
    }

    //判断json的大小以决定是哪个数据
    if(bytes.size() <= 500)
    {   //今日数据 只有一天，数据量小
        parseToday(bytes);
    }
    else
    {   //预报信息，一共四天，数据量大
        parseJson(bytes);
    }


}

void Widget::slot_exitApp()
{
    qApp->exit(0);  //return 0;
}

//请求数据
void Widget::getWeatherInfo(QNetworkAccessManager* manager, bool isToday)
{
    QString citycode = tool[city];
    if(citycode == "000000" || citycode == "000000000")
    {
        QMessageBox::warning(this, u8"错误", u8"天气: 指定城市不存在!", QMessageBox::Ok);
        return;
    }
    QUrl* jsonUrl;
    if(isToday)
    {   //查询当天数据，没有extension扩展
        jsonUrl = new QUrl(url + citycode + key);
    }
    else
    {   //查询预报数据，加上extension扩展，区别在于预报数据没有当天数据详细
        jsonUrl = new QUrl(url + citycode + key + extensions);
    }

    qDebug() << "JsonUrl = " << jsonUrl->toString();

    manager->get(QNetworkRequest(*jsonUrl));
}

//解析json数据
void Widget::parseJson(QByteArray bytes)
{
    QJsonParseError err;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(bytes, &err);
    if(err.error != QJsonParseError::NoError)
    {   //解析出错
        qDebug() << "解析json出错";
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    QString message = jsonObj.value("info").toString();
    if(message.contains("OK") == false)
    {
        QMessageBox::information(this, tr("The information of json_desc"), u8"天气: 城市错误!", QMessageBox::Ok);
        city = cityTmp;
        return;
    }
    //phase1 clear！ 现在已经成功访问网页，成功读取json数据，下面就差解析数据了
    //几乎所有数据都在lives数组里面
    //forecasts是一个数组，[0]是一个Json对象
    //casts是一个数组，其中每一个都是一个json对象，一共四个 分别对应从今天开始的四天
    QJsonArray forecasts = jsonObj.value("forecasts").toArray(); //forecasts数组，现在的单次查询只有一个
    QJsonObject jsonProvince = forecasts[0].toObject(); //根数组的第一项，是个对象，包含一些基础的信息和一个数组
    //jsonProvince 下有一个city
    //jsonProvince 有一个报道时间

    //然后是下一个数组
    QJsonArray casts = jsonProvince.value("casts").toArray();
    qDebug() << "Array size = " << casts.count();
    //循环遍历，一共四天
    for (int i = 0; i < 4; i++)
    {
        QJsonObject castObj = casts.at(i).toObject();
        forecast[i].week = u8"星期" + castObj.value("week").toString();      //周几
        forecast[i].date = castObj.value("date").toString();                //日期
        forecast[i].power = castObj.value("daypower").toString();           //风力
        forecast[i].type = castObj.value("dayweather").toString();          //天气状况
        forecast[i].high = castObj.value("daytemp").toString();             //最高气温
        forecast[i].low = castObj.value("nighttemp").toString();            //最低气温
    }
    //获取当日详细数据 getWeatherInfo会循序调用一直到parseToday，解析完毕今日数据并保存
    getWeatherInfo(manager, true);
    setLabelConnect();  //预报数据的解析和当日详细数据的解析分开保存，否则在函数执行完析构的时候数据保存不成功
}
void Widget::parseToday(QByteArray bytes)
{
    QJsonParseError err;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(bytes, &err);
    if(err.error != QJsonParseError::NoError)
    {   //解析出错
        qDebug() << "解析json出错";
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    QString message = jsonObj.value("info").toString();
    if(message.contains("OK") == false)
    {
        QMessageBox::information(this, tr("The information of json_desc"), u8"天气: 城市错误!", QMessageBox::Ok);
        city = cityTmp;
        return;
    }
    //总的json中，当日天气信息的数据存放在lives数组中 该数组只有一个数据
    QJsonArray lives = jsonObj.value("lives").toArray();
    QJsonObject jsonTodayInfo = lives.at(0).toObject();
    //现在的jsonTodayInfo中存放的就是当日数据
    today.province = jsonTodayInfo.value("province").toString();        //省份
    today.city = jsonTodayInfo.value("city").toString();                //城市
    today.type = jsonTodayInfo.value("weather").toString();             //天气类型
    today.tempature = jsonTodayInfo.value("temperature").toString() + u8"°";//温度
    today.fx = jsonTodayInfo.value("winddirection").toString();         //风向
    today.fl = jsonTodayInfo.value("windpower").toString();             //风力
    today.humidity = jsonTodayInfo.value("humidity").toString();        //湿度
    today.date = jsonTodayInfo.value("reporttime").toString();          //日期

    setTodayConnect();
}

//设置控件文本
void Widget::setLabelConnect()
{
    //全四天的数据
    for (int i = 0; i < 4; ++i)
    {
        forecast_week_list[i]->setText(forecast[i].week);
        forecast_date_list[i]->setText(forecast[i].date.right(5));
        forecast_type_list[i]->setText(forecast[i].type);
        forecast_high_list[i]->setText(forecast[i].high);
        forecast_low_list[i]->setText(forecast[i].low);
        forecast_typeIco_list[i]->setStyleSheet(tr("image: url(:/day/%1.png);").arg(forecast[i].type));

        //设置风力等级  如此设置是为了方便改变颜色
        if (forecast[i].power.toInt() >= 0 && forecast[i].power.toInt() <= 3)
        {
            forecast_aqi_list[i]->setText(forecast[i].power + u8"级");
            forecast_aqi_list[i]->setStyleSheet("color: rgb(0, 255, 0);");
        }
        else if (forecast[i].power.toInt() > 3 && forecast[i].power.toInt() <= 5)
        {
            forecast_aqi_list[i]->setText(forecast[i].power + u8"级");
            forecast_aqi_list[i]->setStyleSheet("color: rgb(255, 255, 0);");
        }
        else if (forecast[i].power.toInt() > 5 && forecast[i].power.toInt() <= 8)
        {
            forecast_aqi_list[i]->setText(forecast[i].power + u8"级");
            forecast_aqi_list[i]->setStyleSheet("color: rgb(255, 170, 0);");
        }
        else if (forecast[i].power.toInt() > 8 && forecast[i].power.toInt() <= 10)
        {
            forecast_aqi_list[i]->setText(forecast[i].power + u8"级");
            forecast_aqi_list[i]->setStyleSheet("color: rgb(255, 0, 0);");
        }
        else
        {
            forecast_aqi_list[i]->setText(forecast[i].power + u8"级");
            forecast_aqi_list[i]->setStyleSheet("color: rgb(170, 0, 0);");
        }
    } //end of for loop

    //TODO：这里是画线的，先不调用
    //ui->curveLb->update();
}
void Widget::setTodayConnect()
{
    qDebug() << u8"今日详细数据：\n" << today.province << "\n" << today.city << "\n" << today.type << "\n" << today.tempature << "\n" << today.fx << "\n" << today.fl << "\n" << today.humidity << "\n" << today.date;
    //今日数据
    ui->provinceLb->setText(today.province);    //省份
    ui->cityLb->setText(today.city);            //城市
    ui->typeLb->setText(today.type);            //天气类型
    ui->temLb->setText(today.tempature);        //温度
    ui->fxLb->setText(today.fx);                //风向
    ui->flLb->setText(today.fl);                //风力
    ui->shiduLb->setText(today.humidity);       //湿度
    ui->dateLb->setText(today.date.left(10));   //日期
    //ui->ganmaoBrowser->setText(u8"极少数敏感人群应减少户外活动");

    //判断白天还是夜晚的图标
    QString sunsetTime = today.date.left(10) + " " + today.sunset;  //sunset设置为19:00
    if(QDateTime::currentDateTime() <= QDateTime::fromString(sunsetTime, "yyyy-MM-dd hh:mm"))
    {
        ui->typeIcoLb->setStyleSheet(tr("border-image: url(:/day/%1.png); background-color: argb(60, 60, 60, 0);").arg(today.type));
    }
    else
    {
        ui->typeIcoLb->setStyleSheet(tr("border-image: url(:/night/%1.png); background-color: argb(60, 60, 60, 0);").arg(today.type));
    }

    ui->qualityLb->setText(today.quality == "" ? "无" : today.quality);
    if (today.quality == u8"还行") //默认值只有这一个
    {
        ui->qualityLb->setStyleSheet("color: rgb(0, 255, 0); background-color: argb(255, 255, 255, 0);");
    }
    else
    {
        ui->qualityLb->setStyleSheet("color: rgb(255, 255, 255); background-color: argb(255, 255, 255, 0);");
    }
}

//日出日落图形绘制
void Widget::paintSunRiseSet()
{
}

//温度曲线绘制
void Widget::paintCurve()
{
    QPainter painter(ui->curveLb);
    painter.setRenderHint(QPainter::Antialiasing, true); //反锯齿

    //将温度转换为int类型，并计算平均值，平均值作为curveLb曲线的参考值，参考Y坐标为45
    int tempTotal = 0;
    int high[4] = {};
    int low[4] = {};

    QString h, l;
    for (int i = 0; i < 4; ++i)
    {
        h = forecast[i].high;
        high[i] = (int)(h.toDouble());
        tempTotal += high[i];

        l = forecast[i].low;
        low[i] = (int)(l.toDouble());
    }
    int tempAverage = (int)(tempTotal / 4); //最高温平均值

    //算出温度对应坐标
    int pointX[4] = {50, 152, 252, 352}; // 点的X坐标
    int pointHY[4] = {0};
    int pointLY[4] = {0};
    for (int i = 0; i < 4; ++i)
    {
        pointHY[i] = TEMPERATURE_STARTING_COORDINATE - ((high[i] - tempAverage) * SPAN_INDEX);
        pointLY[i] = TEMPERATURE_STARTING_COORDINATE + ((tempAverage - low[i]) * SPAN_INDEX);
    }

    QPen pen = painter.pen();
    pen.setWidth(1);

    //高温曲线绘制
    painter.save();
    pen.setColor(QColor(255,170,0));
    pen.setStyle(Qt::DotLine);
    painter.setPen(pen);
    painter.setBrush(QColor(255,170,0));
    painter.drawEllipse(QPoint(pointX[0], pointHY[0]), ORIGIN_SIZE, ORIGIN_SIZE);
    painter.drawEllipse(QPoint(pointX[1], pointHY[1]), ORIGIN_SIZE, ORIGIN_SIZE);
    painter.drawLine(pointX[0], pointHY[0], pointX[1], pointHY[1]);

    pen.setStyle(Qt::SolidLine);
    pen.setWidth(1);
    painter.setPen(pen);
    for (int i = 1; i < 3; ++i)
    {
        painter.drawEllipse(QPoint(pointX[i+1], pointHY[i+1]), ORIGIN_SIZE, ORIGIN_SIZE);
        painter.drawLine(pointX[i], pointHY[i], pointX[i+1], pointHY[i+1]);
    }
    painter.restore();

    //低温曲线绘制
    pen.setColor(QColor(0, 255, 255));
    pen.setStyle(Qt::DotLine);
    painter.setPen(pen);
    painter.setBrush(QColor(0, 255, 255));
    painter.drawEllipse(QPoint(pointX[0], pointLY[0]), ORIGIN_SIZE, ORIGIN_SIZE);
    painter.drawEllipse(QPoint(pointX[1], pointLY[1]), ORIGIN_SIZE, ORIGIN_SIZE);
    painter.drawLine(pointX[0], pointLY[0], pointX[1], pointLY[1]);

    pen.setColor(QColor(0, 255, 255));
    pen.setStyle(Qt::SolidLine);
    painter.setPen(pen);
    for (int i = 1; i < 3; i++)
    {
        painter.drawEllipse(QPoint(pointX[i+1], pointLY[i+1]), ORIGIN_SIZE, ORIGIN_SIZE);
        painter.drawLine(pointX[i], pointLY[i], pointX[i+1], pointLY[i+1]);
    }
}

void Widget::callKeyBoard()
{
    //nothing
}

