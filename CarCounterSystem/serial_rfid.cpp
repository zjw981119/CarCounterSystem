#define _CRT_SECURE_NO_WARNINGS

#include "json.hpp"
#include <WzSerialPort.h>

#include <iostream>
#include <fstream>
#include <io.h>
#include <queue>
#include <list>
#include <string.h>
#include <map>
#include <cpr/cpr.h>
#include <thread>
#include <ctime>
#include <windows.h>
#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>


#include <xlnt/xlnt.hpp>
//#include <SerialPort.h>



using namespace cv;
using namespace std;
using json = nlohmann::json;

void create_rfid_carNum_file(map<string,string> dic_rfid_carNum) {
	string filename = "rfid_carNum.xlsx";
	//�鿴�ļ��Ƿ����
	if (_access(filename.c_str(),00)!=-1)
	{
		//������ɾ��
		remove(filename.c_str());
		cout << "���ڸ��ļ�����ɾ��" << endl;
	}
	else
		cout << "�ļ�������" << endl;

	try
	{
		xlnt::workbook wb;
		xlnt::worksheet mainSheet = wb.active_sheet();
		//mainSheet.title("abc"); ����
		mainSheet.cell("A1").value("rfid");
		mainSheet.cell("B1").value("carNum");
		//�����ñ���Ϣд��xlsx�����
		int row_num = 2;
		for (map<string, string>::iterator it = dic_rfid_carNum.begin(); it != dic_rfid_carNum.end(); it++)
		{
			mainSheet.cell(1,row_num).value(it->first);
			mainSheet.cell(2,row_num).value(it->second);
			row_num+=1;
		}

		//����xlsx�ļ�
		wb.save("rfid_carNum.xlsx");
	}
	catch (std::exception e)
	{
		std::string s = e.what();
		cout << s << endl;
	}


}

void get_rfid_confg_from_request() {
	int request_times = 0;
	map<string, string> dic_rfid_carNum;
	while (request_times < 5)
	{
		//����http�����Ƿ���Ч�õ���վhttps://www.httpbin.org/get
		cpr::Response r = cpr::Get(cpr::Url{ "http://110.17.165.146:7080/GuangNaReceived/counter/initConfig?address=GNXM" });
		//��ȡdata�����е��ַ�����ת��Ϊjson��ʽ
		string s = r.text;
		auto config_json = json::parse(s);
		if (config_json["result"]["success"] == true)
		{
			auto record = json::parse(string(config_json["data"]));
			//cout << record << endl;
			//rfid���źͳ��Ŷ�Ӧ��ϵתΪ�ֵ�
			for (json::iterator it = record.begin(); it != record.end(); it++) {
				dic_rfid_carNum.insert(pair<string, string>(it.key(), it.value()));
			}
			break;
			//cout << dic_rfid_carNum["68359937"] << endl;
		}
		else
		{
			request_times += 1;
			continue;
		}
	}

	if (request_times > 5)
	{
		cout << "��������ʧ��" << endl;
	}
	//���������ȡ������Ϣ�ɹ�����������Ϣ�����ڱ���xlsx�ļ���
	else
	{
		create_rfid_carNum_file(dic_rfid_carNum);
		cout << "����rfid���ű���ɹ�" << endl;
	}
	


	//string config_json = r.dump();
	//cout << config_json;
}

void ini_cap(VideoCapture &cap) {
	// url = 'rtsp://admin:gn123456@222.74.94.190:1554/Streaming/Channels/0301?transportmode=multicas'
    //url = 'rtsp://admin:gn123456@192.168.12.143:554/h264/ch1/main/av_stream'
    //url = 'rtsp://admin:gn123456@192.168.1.161:554/Streaming/Channels/1'
	int url = 0;
	cap.open(0); //������ͷ������Ĭ��0
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	cap.set(CV_CAP_PROP_FPS, 20);
	if (cap.isOpened()){
		cout << "����ͷ�Ѵ�" << endl;
	}
}

//��ȡ������ǰʱ�䲢���ַ�����ʽ����
string GetSystemTime()
{
	SYSTEMTIME m_time;
	GetLocalTime(&m_time);
	char szDateTime[100] = { 0 };
	sprintf(szDateTime, "%02u-%02u-%02u %02u+%02u+%02u", m_time.wYear, m_time.wMonth,
		m_time.wDay, m_time.wHour, m_time.wMinute, m_time.wSecond);
	string time(szDateTime);
	return time;
}

void rec_serialdata() {
	// �򿪴���,�ɹ�����true��ʧ�ܷ���false
	// portname(������): ��Windows����"COM1""COM2"�ȣ���Linux����"/dev/ttyS1"��
	// baudrate(������): 9600��19200��38400��43000��56000��57600��115200 
	// parity(У��λ): 0Ϊ��У�飬1Ϊ��У�飬2ΪżУ�飬3Ϊ���У�飨��������windows)
	// databit(����λ): 4-8(windows),5-8(linux)��ͨ��Ϊ8λ
	// stopbit(ֹͣλ): 1Ϊ1λֹͣλ��2Ϊ2λֹͣλ,3Ϊ1.5λֹͣλ
	// synchronizeflag(ͬ�����첽,��������windows): 0Ϊ�첽��1Ϊͬ��
	//bool open(const char* portname, int baudrate, char parity, char databit, char stopbit, char synchronizeflag=1);
	WzSerialPort serialport;
	
	if (serialport.open("COM4", 57600, 0, 8, 1))
	{
		cout << "�����Ѵ�" << endl;
		//cout << "test" << endl;
		unsigned char buf[1024];
		try {
			while (true)
			{
				memset(buf, 0, 1024);
				//Sleep(1 * 1000);

				serialport.receive(buf, 1024);
				cout << "test" << endl;
				int tmp = serialport.receive(buf, 1024);
				cout << "���ݳ���Ϊ: " << tmp << endl;
				//����ַ���ӡ
				for (int i = 0; i < tmp; i++)
				{
					cout << buf[i];
				}
				cout << endl;
			}
		}
		
		catch (std::exception e)
		{
			std::string s = e.what();
			cout << s << endl;
		}
	}
	else {
		cout << "���ڴ�ʧ��" << endl;
	}
	
	
}

//���մ������ݲ�������ͷ������Ƭ
void rev_serial(queue<vector<string>> &picinfo,queue<Mat> &pic) {
	VideoCapture cap;
	ini_cap(cap);
	
	double width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	double height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	cout << "��" << width << " ��" << height << endl;
	
	int times = 0;
	
	string flag;
	string cap_time;
	string now_time="��ûд";
	vector<string> record;
	Mat frame;
	Mat blankframe(480, 640, CV_8UC1, Scalar(255));
	
	while (times<5)
	{
		cap >> frame;
		//��ȡ����ʱ��
		cap_time = GetSystemTime();
		cout << cap_time << endl;
		//imshow("����", blankframe);
		//waitKey(0);
		
		if (!frame.empty())
		{
			flag = "cap";
			break; //ץͼ�ɹ���ֹͣץͼ
		}
		times += 1;
     }
	//ץͼʧ�ܣ����ɿհ�ͼƬ
	if (times == 5) 
	{
		flag = "blank";
		frame = blankframe;
	}
	cap.release();
	
	string rfidNum = "68359998";
	
	record.push_back(rfidNum);
	record.push_back(now_time);
	record.push_back(cap_time);
	record.push_back(flag);

	picinfo.push(record);

	

	pic.push(frame);
	/*
	while(!picinfo.empty())
	{
		string tmp = picinfo.front();
		cout<<tmp<< endl;
		picinfo.pop();
	}
	*/
	
}

//ͼƬ�����ڱ���
void sav_data(queue<vector<string>>& picinfo, queue<Mat>& pic) {
	vector<string> record;
    while (true)
	{
		string pic_name = "GNXM@001#";
		record.clear();
		Sleep(2 * 1000);
		if (picinfo.empty())
		{
			cout << "�����ѿ�" << endl;
			continue;
		}
		//ȡ����������Ƭ��Ϣ
		record = picinfo.front();
		picinfo.pop();
		//��Ƭ���ƣ�������ð��
		pic_name = pic_name + record[0] + "#" + record[2] + "#" + record[3];
		cout << pic_name << endl;
		
		Mat frame = pic.front();
		pic.pop();
		//����ͼƬ������,����ͼƬѹ������
		vector<int> compression_params;
		compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
		compression_params.push_back(95);
		imwrite("C:/Users/Hasee/Desktop/testPic/" + pic_name +".jpg", frame, compression_params);
		/*
		imshow("����", frame);
		waitKey(0);
		*/
		
	}
}

vector<string> splitstr(string& str, const char *delim)
{
	//stringתΪchar
	char* strc = new char[strlen(str.c_str()) + 1];
	strcpy(strc, str.c_str());

	vector<string> resultVec;
	//��delim�ַ��ָ��ַ��������ֱ�װ��vector<string>������
	char* tmpStr = strtok(strc, delim);
	while (tmpStr != NULL)
	{
		resultVec.push_back(string(tmpStr));
		tmpStr = strtok(NULL, delim);
	}
	
	//delete[] strc;
	return resultVec;
}

void uplode_data() {
	intptr_t handle;
	_finddatai64_t fileInfo;
	vector<string> files;
	vector<string> rfid_record;
	string path = "C:\\Users\\Hasee\\Desktop\\testPic\\*.jpg";
	const char delim[2] = "#";
	handle = _findfirst64(path.c_str(), &fileInfo);
	if (handle==-1)
	{
		cout << "�ļ�����û�ж�ȡ���ļ�" << endl;
	}
	//����ָ��·�����ļ������ļ����ơ���С������ʱ���
	do
	{
		cout << "�ļ���СΪ "<< fileInfo.size<<"�ļ���Ϊ "<<fileInfo.name << endl;
		string picname = fileInfo.name;
		files.push_back(picname);
		//��ͼƬ������"#"�ָ������������
		//�ļ������ָ�ʽ��GNXM@001#68359998#2021-01-17 18+10+50#cap.jpg
		rfid_record=splitstr(picname, delim);
		//�����������Ϊ"GNXM@001", "34087686", "2020-07-14 16:45:54" , "cap.jpg"
		


	} while (_findnext64(handle, &fileInfo) != -1);
		cout << "���ҵ�" << files.size() << "���ļ�" << endl;
}

int main() {


	
	//string s ="b'\x11\x00\xee\x00\xe0 \x820b6\xaa \x04\x13\x13\xfa\xc4\\'";
	//string s = "c45c1100e";
	//cout << "�ַ�������Ϊ�� " << s.length() << endl;
	//s.erase(s.length() - 1, 1);
	//s.erase(0, 2);
	
	//cout << s << endl;


	//get_rfid_confg_from_request();
	queue<vector<string>> picinfo;
	queue<Mat> pic;
	//�����ã���Ҫ��ref()���а�װ
	thread th1 = thread(rev_serial,std::ref(picinfo),ref(pic));
	thread th2 = thread(sav_data, std::ref(picinfo), ref(pic));
	//uplode_data();
	th1.join();
	th2.join();
	



	return 0;
}