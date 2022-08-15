#pragma once
#include <QtSerialPort>
#include <QtCore>
/*������Ϊ����QtAPI�ĳ���Ƶ��дģ��R200��ͨ��API���Խ�Ϊ����Qt�ķ�ʽ���п���д
Ҫʹ�ñ�ģ�飬��Ҫʹ��Qt5��Qt6����������serialportģ��
���ļ�����������Ҫ�����࣬��һ����ΪQtR200Port���ṩ�������豸����ͨ�Ź���
�ڶ�����ΪQtR200Card����QtR200Port��ȡ��һ����֮��Ὠ�������ʵ������ʵ���������û��ֶ��ͷţ�ʮ�ֲ����飩
Ҳ��������ΪQtR200Port��ȡ������֮���ͷţ�Ҫ�����Զ��ͷţ�����ǰ���ù��ڲ���������5���������ɨ�費������ڣ��ȴ�����*/

class QtR200Card :public QObject
{
public:
	QByteArray EPCHex;
	int OutDateCount;
	QtR200Card(QByteArray Hex, int count) {
		EPCHex = Hex;
		OutDateCount = count;
		qDebug() << "CARD : " + Hex;
	}
	~QtR200Card() {
		qDebug() << "REMOVE : " +EPCHex;
	}
};
class QtR200Port :public QObject
{
	Q_OBJECT
signals:
	void received(QByteArray);
public:
	enum class Power
	{
		P18D5 = 0,
		P20D0 = 1,
		P21D5 = 2,
		P23D0 = 3,
		P24D5 = 4,
		P26D0 = 5
	};
	
private:
	bool INDEBUG = false;
	bool OPEN = false;
	QSerialPort* PORT;
	QString PORTNAME;
	QByteArray TEMP;
	QByteArray RAWCOMMAND;
	QString COMMAND;
	QList<QtR200Card*> CARDLIST;
	QStringList COMMANDSPLIT;
	QSerialPort::BaudRate BAUDRATE = QSerialPort::BaudRate::Baud115200;
	QSerialPort::DataBits DATABITS = QSerialPort::DataBits::Data8;
	QSerialPort::StopBits STOPBITS = QSerialPort::StopBits::OneStop;
	QSerialPort::Parity PARITY = QSerialPort::Parity::NoParity;
	QSerialPort::FlowControl FLOWCONTROL = QSerialPort::FlowControl::NoFlowControl;
	unsigned short CARDALIVECOUNT = 5;
	QMutex mutex;

public:
	QtR200Port(QObject* parent = Q_NULLPTR) {
		this->setParent(parent);
		PORT = new QSerialPort(this);
		PORT->setBaudRate(BAUDRATE);
		PORT->setDataBits(DATABITS);
		PORT->setStopBits(STOPBITS);
		PORT->setParity(PARITY);
		PORT->setFlowControl(FLOWCONTROL);
	}
public slots:
	void setPortName(QString portName) {
		PORT->setPortName(portName);
		PORTNAME = portName;
	}
	void setBaudRate(QSerialPort::BaudRate rate)
	{
		PORT->setBaudRate(rate);
		BAUDRATE = rate;
	}
	void setDataBits(QSerialPort::DataBits bits)
	{
		PORT->setDataBits(bits);
		DATABITS = bits;
	}
	void setStopBits(QSerialPort::StopBits bits)
	{
		PORT->setStopBits(bits);
		STOPBITS = bits;
	}
	void setParity(QSerialPort::Parity parity)
	{
		PORT->setParity(parity);
		PARITY = parity;
	}
	void setFlowControl(QSerialPort::FlowControl flow)
	{
		PORT->setFlowControl(flow);
		FLOWCONTROL = flow;
	}
	void setAliveCount(unsigned short count) {
		CARDALIVECOUNT = count;
	}
	void sendData(const char* c, int l) {
		QByteArray i = QByteArray::fromRawData(c, l);
		if (INDEBUG) {qDebug() <<"QtR200Port send :" + i.toHex(); }
		PORT->write(i);
	}
	void open(QIODevice::OpenModeFlag flag = QIODevice::ReadWrite) {
		if (OPEN) {
			return;
		}
		if (PORT->open(flag)) {
			OPEN = true;
			if (INDEBUG) {qDebug() <<"QtR200Port open success"; }
			connect(PORT, SIGNAL(readyRead()), this, SLOT(getData()));
		}
		else {
			if (INDEBUG) {qDebug() <<"QtR200Port open failed"; }
		}
	}
	void close() {
		if (OPEN) {
			PORT->close();
			OPEN = false;
			if (INDEBUG) {qDebug() <<"QtR200Port close success"; }
		}
	}
	void getData(void) {
		TEMP += PORT->readAll();
		QString TEMPSTR = QString(TEMP.toHex());
		//qDebug() << TEMPSTR.toUpper();
		if (TEMPSTR.contains( "7ebb")) {
			QList<QByteArray> List = TEMP.split('\x7e\xbb');
			TEMP = List[0] + "\x7e";
			emit received(TEMP);
			RAWCOMMAND = (TEMP).toHex();
			COMMAND = QString(RAWCOMMAND);
			//qDebug() << COMMAND;
			COMMANDSPLIT.clear();
			for (int i = 0; i < COMMAND.length(); i += 2) {
				if (i + 2 > COMMAND.length()) { return; }
				COMMANDSPLIT.append(COMMAND.mid(i, 2));
			}
			analysisCommand();
			TEMP = List[1];
		}
	}
	void readSingle() {
		sendData("\xBB\x00\x22\x00\x00\x22\x7E", 7);
	}
	void readMulti() {
		sendData("\xBB\x00\x27\x00\x03\x22\xFF\xFF\x4A\x7E", 10);
	}
	void stopMulti() {
		sendData("\xBB\x00\x28\x00\x00\x28\x7E", 7);
	}
	void setPower(Power power) {
		switch (power) {
		case Power::P18D5:
			sendData("\xBB\x00\xB6\x00\x02\x04\xE2\x9E\x7E", 9);
			break;
		case Power::P20D0:
			sendData("\xBB\x00\xB6\x00\x02\x05\x78\x35\x7E", 9);
			break;
		case Power::P21D5:
			sendData("\xBB\x00\xB6\x00\x02\x06\x0E\xCC\x7E", 9);
			break;
		case Power::P23D0:
			sendData("\xBB\x00\xB6\x00\x02\x06\xA4\x62\x7E", 9);
			break;
		case Power::P24D5:
			sendData("\xBB\x00\xB6\x00\x02\x07\x3A\xF9\x7E", 9);
			break;
		case Power::P26D0:
			sendData("\xBB\x00\xB6\x00\x02\x07\xD0\x8F\x7E", 9);
			break;
		}
	}
private:
	void analysisCommand() {
		//#qDebug() << COMMANDSPLIT;
		QString CommandType = COMMANDSPLIT[2];
		if (CommandType == "22" || COMMAND=="bb01ff000115167e") {
			this->createCard();
		}
	}
	void createCard() {
		QString EPC = COMMANDSPLIT.mid(8, 12).join(" ").toUpper();
		QByteArray EPCHex = RAWCOMMAND.mid(16, 24);
		for (int i = 0; i < CARDLIST.length(); i++) {
			QtR200Card* card = CARDLIST[i];
			card->OutDateCount--;
			if (card->OutDateCount <= 0) {
				card->deleteLater();
				CARDLIST.removeAt(i);
				continue;
			}
			if (card->EPCHex == EPCHex) {
				card->OutDateCount = CARDALIVECOUNT;
				return;
			}
		}
		if (COMMANDSPLIT[2] == "22" && EPC != "00 00 00 00 00 00 00 00 00 00 00 00") {
			CARDLIST.append(new QtR200Card(EPCHex, CARDALIVECOUNT));
		}
	}
};