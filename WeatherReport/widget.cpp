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

#define SPAN_INDEX 3  //�¶����߼��ָ��
#define ORIGIN_SIZE 3 //�¶�����Զ���С
#define TEMPERATURE_STARTING_COORDINATE 25  //����ƽ��ֵ��ʼ����

//�ճ��������
const QPoint Widget::sun[2] = {
    QPoint(20,75),
    QPoint(130,75)
};

//�ճ������¼�
const QRect Widget::sunRiseSet[2] = {
    QRect(0,80,50,20),
    QRect(100,80,50,20)
};

//�ճ�����Բ��
const QRect Widget::rect[2] = {
    QRect(25,25,100,100),   //����Բ��
    QRect(50,80,50,20)      //"�ճ�����"�ı�
};

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    this->setWindowFlag(Qt::FramelessWindowHint); //���ر߿�

    ui->cityLineEdit->setStyleSheet("QLineEdit{border: 1px solid gray; border-radius: 4px; background:rgb(85, 85, 127); color:rgb(255, 255, 255);} QLineEdit:hover{border-color:rgb(101, 255, 106); }");
    ui->refreshBt->setStyleSheet("QPushButton{border: 2px solid gray; border-radius: 4px;} QPushButton:hover{border-color:rgb(0, 255, 106); }");


    forecast_week_list << ui->week1Lb << ui->week2Lb << ui->week3Lb << ui->week4Lb;
    forecast_date_list << ui->date1Lb << ui->date2Lb << ui->date3Lb << ui->date4Lb;
    forecast_aqi_list << ui->quality1Lb << ui->quality2Lb << ui->quality3Lb << ui->quality4Lb;
    forecast_type_list << ui->type1Lb << ui->type2Lb << ui->type3Lb << ui->type4Lb;
    forecast_typeIco_list << ui->typeIco1Lb << ui->typeIco2Lb << ui->typeIco3Lb << ui->typeIco4Lb;
    forecast_high_list << ui->high1Lb << ui->high2Lb << ui->high3Lb << ui->high4Lb;
    forecast_low_list << ui->low1Lb << ui->low2Lb << ui->low3Lb << ui->low4Lb;

    //�ṹ���ʼ�� ������ϸ����
    today.province = "null";
    today.city = "null";
    today.type = u8" ������ ";
    today.tempature = "null";
    today.fx = u8" ������ ";
    today.fl = u8" ������ ";
    today.humidity = u8" ������ ";
    today.date = "0000-00-00";

    today.quality = u8"����";
    today.sunrise = "00:00";
    today.sunset = "19:00";

    for (int i = 0; i < 4; ++i)
    {
        forecast[i].week = "����0";          //�ܼ�
        forecast[i].date = "yyyy-MM-dd";    //����
        forecast[i].power = "8";            //����
        forecast[i].type = "undefined";     //����״��
        forecast[i].high = " ���� 0.0��C";    //�������
        forecast[i].low = " ���� 0.0��C";     //�������
    }

    //�Ҽ��˵�
    m_pMenu = new QMenu(this);
    m_pExitAct = new QAction;
    m_pExitAct->setText(u8" �˳� ");
    m_pExitAct->setIcon(QIcon(":/icons/close.ico"));
    m_pMenu->addAction(m_pExitAct);
    connect(m_pExitAct, SIGNAL(triggered(bool)), this, SLOT(slot_exitApp()));

    //dateLb��weekLb��ʽ������
    for (int i = 0; i < 4; ++i)
    {
        forecast_date_list[i]->setStyleSheet("background-color: rgba(0, 255, 255, 100);");
        forecast_week_list[i]->setStyleSheet("background-color: rgba(0, 255, 255, 100);");
    }

    //��������API��Ϣ
    //�ߵ�����APIʹ�ã� jsonUrl = url + city_code + key
    //ԭ����https://  �������ʹ��https���ڴ���ʱ��һ����ܣ���������Ҫ�õ�opensslȥ����������qt����������(������Ҫ�Լ���һ��dll����)
    //���Ըĳ�http,��Ȼ��վ����ʾ����ȫ���Ͼ���������Կ�һ��÷Ǽ��ܵģ��������벻�����鷳���ֶ�ȥװopenssl��������������
    url = "http://restapi.amap.com/v3/weather/weatherInfo?city=";
    key = "&key=440d2191d8abbf736b3e24e18ec71488";
    extensions = "&extensions=all";
    //usage: jsonUrl = url + citycode + key + extensions  ���ؽ���ǰ����������ڵ������������Ϣ

    city = u8"����";
    cityTmp = city;
    manager = new QNetworkAccessManager(this);
    connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(replayFinished(QNetworkReply*)));
    getWeatherInfo(manager, false);

    //�¼�����
    ui->sunRiseSetLb->installEventFilter(this); //�����¼�������
    ui->curveLb->installEventFilter(this);
    ui->cityLineEdit->installEventFilter(this);
}

Widget::~Widget()
{
    delete ui;
}

//�¼�����
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
        callKeyBoard();  //���������
    }
    return QWidget::eventFilter(watched, event); //�Ͻ��������������
}

//�����ƶ�
void Widget::mouseMoveEvent(QMouseEvent* event)
{
    this->move(event->globalPos() - mPos);
}

void Widget::mousePressEvent(QMouseEvent* event)
{
    mPos = event->globalPos() - this->pos();
}

//�Ҽ��˵�
void Widget::contexMenuEvent(QContextMenuEvent* menuEvent)
{
    m_pMenu->exec(QCursor::pos());
    menuEvent->accept();
}

//������ť
void Widget::on_searchBt_clicked()
{
    cityTmp = city;
    city = ui->cityLineEdit->text();
    getWeatherInfo(manager, false);
}

//ˢ�°�ť
void Widget::on_refreshBt_clicked()
{
    getWeatherInfo(manager, false);
    //ui->curveLb->update();   TODO:���ߵ��Ȳ�����
}

//��ȡ��Ӧ����Ϣ
void Widget::replayFinished(QNetworkReply* reply)
{
    QByteArray bytes = reply->readAll();

    qDebug() << "bytes size: " << bytes.size();
    if(bytes.size() == 0)
    {
        QMessageBox::warning(this, u8"����", u8"����:�������ݴ���������������!", QMessageBox::Ok);
        return;
    }

    //�ж�json�Ĵ�С�Ծ������ĸ�����
    if(bytes.size() <= 500)
    {   //�������� ֻ��һ�죬������С
        parseToday(bytes);
    }
    else
    {   //Ԥ����Ϣ��һ�����죬��������
        parseJson(bytes);
    }


}

void Widget::slot_exitApp()
{
    qApp->exit(0);  //return 0;
}

//��������
void Widget::getWeatherInfo(QNetworkAccessManager* manager, bool isToday)
{
    QString citycode = tool[city];
    if(citycode == "000000" || citycode == "000000000")
    {
        QMessageBox::warning(this, u8"����", u8"����: ָ�����в�����!", QMessageBox::Ok);
        return;
    }
    QUrl* jsonUrl;
    if(isToday)
    {   //��ѯ�������ݣ�û��extension��չ
        jsonUrl = new QUrl(url + citycode + key);
    }
    else
    {   //��ѯԤ�����ݣ�����extension��չ����������Ԥ������û�е���������ϸ
        jsonUrl = new QUrl(url + citycode + key + extensions);
    }

    qDebug() << "JsonUrl = " << jsonUrl->toString();

    manager->get(QNetworkRequest(*jsonUrl));
}

//����json����
void Widget::parseJson(QByteArray bytes)
{
    QJsonParseError err;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(bytes, &err);
    if(err.error != QJsonParseError::NoError)
    {   //��������
        qDebug() << "����json����";
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    QString message = jsonObj.value("info").toString();
    if(message.contains("OK") == false)
    {
        QMessageBox::information(this, tr("The information of json_desc"), u8"����: ���д���!", QMessageBox::Ok);
        city = cityTmp;
        return;
    }
    //phase1 clear�� �����Ѿ��ɹ�������ҳ���ɹ���ȡjson���ݣ�����Ͳ����������
    //�����������ݶ���lives��������
    //forecasts��һ�����飬[0]��һ��Json����
    //casts��һ�����飬����ÿһ������һ��json����һ���ĸ� �ֱ��Ӧ�ӽ��쿪ʼ������
    QJsonArray forecasts = jsonObj.value("forecasts").toArray(); //forecasts���飬���ڵĵ��β�ѯֻ��һ��
    QJsonObject jsonProvince = forecasts[0].toObject(); //������ĵ�һ��Ǹ����󣬰���һЩ��������Ϣ��һ������
    //jsonProvince ����һ��city
    //jsonProvince ��һ������ʱ��

    //Ȼ������һ������
    QJsonArray casts = jsonProvince.value("casts").toArray();
    qDebug() << "Array size = " << casts.count();
    //ѭ��������һ������
    for (int i = 0; i < 4; i++)
    {
        QJsonObject castObj = casts.at(i).toObject();
        forecast[i].week = u8"����" + castObj.value("week").toString();      //�ܼ�
        forecast[i].date = castObj.value("date").toString();                //����
        forecast[i].power = castObj.value("daypower").toString();           //����
        forecast[i].type = castObj.value("dayweather").toString();          //����״��
        forecast[i].high = castObj.value("daytemp").toString();             //�������
        forecast[i].low = castObj.value("nighttemp").toString();            //�������
    }
    //��ȡ������ϸ���� getWeatherInfo��ѭ�����һֱ��parseToday��������Ͻ������ݲ�����
    getWeatherInfo(manager, true);
    setLabelConnect();  //Ԥ�����ݵĽ����͵�����ϸ���ݵĽ����ֿ����棬�����ں���ִ����������ʱ�����ݱ��治�ɹ�
}
void Widget::parseToday(QByteArray bytes)
{
    QJsonParseError err;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(bytes, &err);
    if(err.error != QJsonParseError::NoError)
    {   //��������
        qDebug() << "����json����";
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();
    QString message = jsonObj.value("info").toString();
    if(message.contains("OK") == false)
    {
        QMessageBox::information(this, tr("The information of json_desc"), u8"����: ���д���!", QMessageBox::Ok);
        city = cityTmp;
        return;
    }
    //�ܵ�json�У�����������Ϣ�����ݴ����lives������ ������ֻ��һ������
    QJsonArray lives = jsonObj.value("lives").toArray();
    QJsonObject jsonTodayInfo = lives.at(0).toObject();
    //���ڵ�jsonTodayInfo�д�ŵľ��ǵ�������
    today.province = jsonTodayInfo.value("province").toString();        //ʡ��
    today.city = jsonTodayInfo.value("city").toString();                //����
    today.type = jsonTodayInfo.value("weather").toString();             //��������
    today.tempature = jsonTodayInfo.value("temperature").toString() + u8"��";//�¶�
    today.fx = jsonTodayInfo.value("winddirection").toString();         //����
    today.fl = jsonTodayInfo.value("windpower").toString();             //����
    today.humidity = jsonTodayInfo.value("humidity").toString();        //ʪ��
    today.date = jsonTodayInfo.value("reporttime").toString();          //����

    setTodayConnect();
}

//���ÿؼ��ı�
void Widget::setLabelConnect()
{
    //ȫ���������
    for (int i = 0; i < 4; ++i)
    {
        forecast_week_list[i]->setText(forecast[i].week);
        forecast_date_list[i]->setText(forecast[i].date.right(5));
        forecast_type_list[i]->setText(forecast[i].type);
        forecast_high_list[i]->setText(forecast[i].high);
        forecast_low_list[i]->setText(forecast[i].low);
        forecast_typeIco_list[i]->setStyleSheet(tr("image: url(:/day/%1.png);").arg(forecast[i].type));

        //���÷����ȼ�  ���������Ϊ�˷���ı���ɫ
        if (forecast[i].power.toInt() >= 0 && forecast[i].power.toInt() <= 3)
        {
            forecast_aqi_list[i]->setText(forecast[i].power + u8"��");
            forecast_aqi_list[i]->setStyleSheet("color: rgb(0, 255, 0);");
        }
        else if (forecast[i].power.toInt() > 3 && forecast[i].power.toInt() <= 5)
        {
            forecast_aqi_list[i]->setText(forecast[i].power + u8"��");
            forecast_aqi_list[i]->setStyleSheet("color: rgb(255, 255, 0);");
        }
        else if (forecast[i].power.toInt() > 5 && forecast[i].power.toInt() <= 8)
        {
            forecast_aqi_list[i]->setText(forecast[i].power + u8"��");
            forecast_aqi_list[i]->setStyleSheet("color: rgb(255, 170, 0);");
        }
        else if (forecast[i].power.toInt() > 8 && forecast[i].power.toInt() <= 10)
        {
            forecast_aqi_list[i]->setText(forecast[i].power + u8"��");
            forecast_aqi_list[i]->setStyleSheet("color: rgb(255, 0, 0);");
        }
        else
        {
            forecast_aqi_list[i]->setText(forecast[i].power + u8"��");
            forecast_aqi_list[i]->setStyleSheet("color: rgb(170, 0, 0);");
        }
    } //end of for loop

    //TODO�������ǻ��ߵģ��Ȳ�����
    //ui->curveLb->update();
}
void Widget::setTodayConnect()
{
    qDebug() << u8"������ϸ���ݣ�\n" << today.province << "\n" << today.city << "\n" << today.type << "\n" << today.tempature << "\n" << today.fx << "\n" << today.fl << "\n" << today.humidity << "\n" << today.date;
    //��������
    ui->provinceLb->setText(today.province);    //ʡ��
    ui->cityLb->setText(today.city);            //����
    ui->typeLb->setText(today.type);            //��������
    ui->temLb->setText(today.tempature);        //�¶�
    ui->fxLb->setText(today.fx);                //����
    ui->flLb->setText(today.fl);                //����
    ui->shiduLb->setText(today.humidity);       //ʪ��
    ui->dateLb->setText(today.date.left(10));   //����
    //ui->ganmaoBrowser->setText(u8"������������ȺӦ���ٻ���");

    //�жϰ��컹��ҹ���ͼ��
    QString sunsetTime = today.date.left(10) + " " + today.sunset;  //sunset����Ϊ19:00
    if(QDateTime::currentDateTime() <= QDateTime::fromString(sunsetTime, "yyyy-MM-dd hh:mm"))
    {
        ui->typeIcoLb->setStyleSheet(tr("border-image: url(:/day/%1.png); background-color: argb(60, 60, 60, 0);").arg(today.type));
    }
    else
    {
        ui->typeIcoLb->setStyleSheet(tr("border-image: url(:/night/%1.png); background-color: argb(60, 60, 60, 0);").arg(today.type));
    }

    ui->qualityLb->setText(today.quality == "" ? "��" : today.quality);
    if (today.quality == u8"����") //Ĭ��ֵֻ����һ��
    {
        ui->qualityLb->setStyleSheet("color: rgb(0, 255, 0); background-color: argb(255, 255, 255, 0);");
    }
    else
    {
        ui->qualityLb->setStyleSheet("color: rgb(255, 255, 255); background-color: argb(255, 255, 255, 0);");
    }
}

//�ճ�����ͼ�λ���
void Widget::paintSunRiseSet()
{
}

//�¶����߻���
void Widget::paintCurve()
{
    QPainter painter(ui->curveLb);
    painter.setRenderHint(QPainter::Antialiasing, true); //�����

    //���¶�ת��Ϊint���ͣ�������ƽ��ֵ��ƽ��ֵ��ΪcurveLb���ߵĲο�ֵ���ο�Y����Ϊ45
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
    int tempAverage = (int)(tempTotal / 4); //�����ƽ��ֵ

    //����¶ȶ�Ӧ����
    int pointX[4] = {50, 152, 252, 352}; // ���X����
    int pointHY[4] = {0};
    int pointLY[4] = {0};
    for (int i = 0; i < 4; ++i)
    {
        pointHY[i] = TEMPERATURE_STARTING_COORDINATE - ((high[i] - tempAverage) * SPAN_INDEX);
        pointLY[i] = TEMPERATURE_STARTING_COORDINATE + ((tempAverage - low[i]) * SPAN_INDEX);
    }

    QPen pen = painter.pen();
    pen.setWidth(1);

    //�������߻���
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

    //�������߻���
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

