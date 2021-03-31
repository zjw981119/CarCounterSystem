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
	//查看文件是否存在
	if (_access(filename.c_str(),00)!=-1)
	{
		//存在则删除
		remove(filename.c_str());
		cout << "存在该文件，已删除" << endl;
	}
	else
		cout << "文件不存在" << endl;

	try
	{
		xlnt::workbook wb;
		xlnt::worksheet mainSheet = wb.active_sheet();
		//mainSheet.title("abc"); 表单名
		mainSheet.cell("A1").value("rfid");
		mainSheet.cell("B1").value("carNum");
		//将配置表信息写入xlsx表格中
		int row_num = 2;
		for (map<string, string>::iterator it = dic_rfid_carNum.begin(); it != dic_rfid_carNum.end(); it++)
		{
			mainSheet.cell(1,row_num).value(it->first);
			mainSheet.cell(2,row_num).value(it->second);
			row_num+=1;
		}

		//保存xlsx文件
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
		//测试http访问是否有效用的网站https://www.httpbin.org/get
		cpr::Response r = cpr::Get(cpr::Url{ "http://110.17.165.146:7080/GuangNaReceived/counter/initConfig?address=GNXM" });
		//提取data数据中的字符串，转换为json格式
		string s = r.text;
		auto config_json = json::parse(s);
		if (config_json["result"]["success"] == true)
		{
			auto record = json::parse(string(config_json["data"]));
			//cout << record << endl;
			//rfid卡号和车号对应关系转为字典
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
		cout << "网络请求失败" << endl;
	}
	//网络请求获取配置信息成功，将配置信息保存在本机xlsx文件中
	else
	{
		create_rfid_carNum_file(dic_rfid_carNum);
		cout << "本地rfid车号保存成功" << endl;
	}
	


	//string config_json = r.dump();
	//cout << config_json;
}

void ini_cap(VideoCapture &cap) {
	// url = 'rtsp://admin:gn123456@222.74.94.190:1554/Streaming/Channels/0301?transportmode=multicas'
    //url = 'rtsp://admin:gn123456@192.168.12.143:554/h264/ch1/main/av_stream'
    //url = 'rtsp://admin:gn123456@192.168.1.161:554/Streaming/Channels/1'
	int url = 0;
	cap.open(0); //打开摄像头，本机默认0
	cap.set(CV_CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CV_CAP_PROP_FRAME_HEIGHT, 480);
	cap.set(CV_CAP_PROP_FPS, 20);
	if (cap.isOpened()){
		cout << "摄像头已打开" << endl;
	}
}

//获取本机当前时间并以字符串格式返回
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
	cout << "宽" << width << " 高" << height << endl;
	
	int times = 0;
	
	string flag;
	string cap_time;
	string now_time="还没写";
	list<string> record;
	Mat frame;
	Mat blankframe(480, 640, CV_8UC1, Scalar(255));
	
	while (times<5)
	{
		cap >> frame;
		//获取本机时间
		cap_time = GetSystemTime();
		cout << cap_time << endl;
		//imshow("测试", blankframe);
		//waitKey(0);
		
		if (!frame.empty())
		{
			flag = "cap";
			break; //抓图成功，停止抓图
		}
		times += 1;
     }
	//抓图失败，生成空白图片
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
			cout << "队列已空" << endl;
			continue;
		}
		record = picinfo.front();
		picinfo.pop();
		//照片名称，不可用冒号
		while (!record.empty())
		{
			string tmp = record.front();
			pic_name = pic_name + tmp + "#";
			record.pop_front();
		}
		cout << pic_name << endl;
		
		Mat frame = pic.front();
		pic.pop();
		//保存图片到本地,设置图片压缩参数
		vector<int> compression_params;
		compression_params.push_back(CV_IMWRITE_JPEG_QUALITY);
		compression_params.push_back(95);
		imwrite("C:/Users/Hasee/Desktop/testPic/" + pic_name +".jpg", frame, compression_params);
		/*
		imshow("测试", frame);
		waitKey(0);
		*/
		
	}
}

int main() {

	get_rfid_confg_from_request();
	
	/*
	queue<list<string>> picinfo;
	queue<Mat> pic;
	//传引用，需要用ref()进行包装
	thread th1 = thread(rev_serial,std::ref(picinfo),ref(pic));
	thread th2 = thread(sav_data, std::ref(picinfo), ref(pic));
	th1.join();
	th2.join();
	*/
	//明确解析
	/*
	string s = " aaa";
	auto j3 = json::parse(s);
	cout << j3 << endl;
	*/

	return 0;
}