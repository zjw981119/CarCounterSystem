#define _CRT_SECURE_NO_WARNINGS

#include "json.hpp"
#include <iostream>
#include <fstream>
#include <io.h>
#include <queue>
#include <list>
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

void rev_serial(queue<list<string>> &picinfo,queue<Mat> &pic) {
	VideoCapture cap;
	ini_cap(cap);
	
	double width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	double height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	cout << "��" << width << " ��" << height << endl;
	
	int times = 0;
	
	string flag;
	string cap_time;
	string now_time="��ûд";
	list<string> record;
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

void sav_data(queue<list<string>>& picinfo, queue<Mat>& pic) {
	list<string> record;
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
		record = picinfo.front();
		picinfo.pop();
		//��Ƭ���ƣ�������ð��
		while (!record.empty())
		{
			string tmp = record.front();
			pic_name = pic_name + tmp + "#";
			record.pop_front();
		}
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

int main() {

	get_rfid_confg_from_request();
	
	/*
	queue<list<string>> picinfo;
	queue<Mat> pic;
	//�����ã���Ҫ��ref()���а�װ
	thread th1 = thread(rev_serial,std::ref(picinfo),ref(pic));
	thread th2 = thread(sav_data, std::ref(picinfo), ref(pic));
	th1.join();
	th2.join();
	*/
	//��ȷ����
	/*
	string s = " aaa";
	auto j3 = json::parse(s);
	cout << j3 << endl;
	*/

	return 0;
}