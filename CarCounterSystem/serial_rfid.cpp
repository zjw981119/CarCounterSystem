#define _CRT_SECURE_NO_WARNINGS

//#include <stdio.h>
//#include <stdlib.h>

#include "json.hpp"
#include <base64.h>
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

//根据获取的rfid车号配置数据，更新本地rfid_carNum配置文件
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

//从server请求中获取rfid和车号对应表
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

//初始化摄像头
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

void rec_serialdata() {
	// 打开串口,成功返回true，失败返回false
	// portname(串口名): 在Windows下是"COM1""COM2"等，在Linux下是"/dev/ttyS1"等
	// baudrate(波特率): 9600、19200、38400、43000、56000、57600、115200 
	// parity(校验位): 0为无校验，1为奇校验，2为偶校验，3为标记校验（仅适用于windows)
	// databit(数据位): 4-8(windows),5-8(linux)，通常为8位
	// stopbit(停止位): 1为1位停止位，2为2位停止位,3为1.5位停止位
	// synchronizeflag(同步、异步,仅适用与windows): 0为异步，1为同步
	//bool open(const char* portname, int baudrate, char parity, char databit, char stopbit, char synchronizeflag=1);
	WzSerialPort serialport;
	
	if (serialport.open("COM4", 57600, 0, 8, 1))
	{
		cout << "串口已打开" << endl;
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
				cout << "数据长度为: " << tmp << endl;
				//逐个字符打印
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
		cout << "串口打开失败" << endl;
	}
	
	
}

//接收串口数据并用摄像头拍摄照片
void rev_serial(queue<vector<string>> &picinfo,queue<Mat> &pic) {
	VideoCapture cap;
	ini_cap(cap);
	
	double width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
	double height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
	cout << "宽" << width << " 高" << height << endl;
	
	int times = 0;
	
	string flag;
	string cap_time;
	string now_time="还没写";
	vector<string> record;
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
		cap_time = GetSystemTime();
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

//图片保存在本地
void sav_data(queue<vector<string>>& picinfo, queue<Mat>& pic) {
	vector<string> record;
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
		//取出队列中照片信息
		record = picinfo.front();
		picinfo.pop();
		//照片名称，不可用冒号
		pic_name = pic_name + record[0] + "#" + record[2] + "#" + record[3];
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

//分割字符串
vector<string> splitstr(string& str, const char *delim)
{
	//string转为char
	char* strc = new char[strlen(str.c_str()) + 1];
	strcpy(strc, str.c_str());

	vector<string> resultVec;
	//以delim字符分割字符串，并分别装入vector<string>容器中
	char* tmpStr = strtok(strc, delim);
	while (tmpStr != NULL)
	{
		resultVec.push_back(string(tmpStr));
		tmpStr = strtok(NULL, delim);
	}
	
	//delete[] strc;
	return resultVec;
}

//字符串替换全部某一指定字符
string replace_all(string str, string preStr, string nextStr)
{
	string::size_type startpos = 0;
	while (startpos != string::npos) //如果没找到，返回一个特别的标志c++中用npos表示
	{
		startpos = str.find(preStr);
		//cout << startpos << endl;
		//替换
		if (startpos != string::npos)
		{
			str.replace(startpos, 1, nextStr);
		}
	}
	return str;
}

string pic_convertto_bin(const char* Filename) {
	//以二进制方式打开文件，将文件数据输入到内存
	ifstream file(Filename, ifstream::in | ifstream::binary);
	file.seekg(0, file.end); //偏移量0，从文件末尾开始偏移
	int file_length = file.tellg();
	cout << file_length << endl;

	file.seekg(0, file.beg);
	char* buffer = new char[file_length];
	file.read(buffer, file_length);
	
	string base64_data = base64_encode(reinterpret_cast<const unsigned char*>(buffer), file_length);
	base64_data = "data:image/jpg;base64," + base64_data;
	//cout << "base64编码长度为； "<<base64_data.length()<<endl<< base64_data << endl;
	//删除buffer,清空指针
	delete[]buffer;
	file.close();
	return base64_data;
	
	/*
	FILE* fp;
	fopen_s(&fp, Filename, "rb"); //以二进制方式打开图像
	if (fp ==NULL)
	{
		perror("File opening failed");
		return EXIT_FAILURE;
	}

	fseek(fp, 0, SEEK_END); //设置指向文件的指针在文件末尾SEEK_END 添加偏移量0
	long int size = ftell(fp);//文件大小
	rewind(fp);//重置文件指针指向文件开头
	cout << "图片大小为： "<< size << endl;

	//根据图像数据长度分配内存buffer
	 char* ImgBuffer = (char*)malloc(size * sizeof(char));
	//将图像数据读入buffer
	fread(ImgBuffer, size, 1, fp);
	fclose(fp);
	if ((fp = fopen("C:\\Users\\Hasee\\Desktop\\a.txt", "wb")) == NULL)
	{
		perror("txtxFile opening failed");
		exit(0);
	}

	fwrite(ImgBuffer, size, 1, fp);
	printf("ok");

	fclose(fp);
	free(ImgBuffer);
	
	string base64_data = base64_encode(string(ImgBuffer), false);
	cout << "图像数据为： " << ImgBuffer << endl;
	cout << "图像base64编码为： " << base64_data << endl;
	*/
	
	
	
	
	
}

//本机图片上传服务器
void uplode_data() {
	intptr_t handle;
	_finddatai64_t fileInfo;
	vector<string> files;
	vector<string> rfid_record;
	string search_path = "C:\\Users\\Hasee\\Desktop\\testPic\\*.jpg";
	string path = "C:\\Users\\Hasee\\Desktop\\testPic\\";
	const char delim[2] = "#";
	handle = _findfirst64(search_path.c_str(), &fileInfo);
	if (handle==-1)
	{
		cout << "文件夹中没有读取到文件" << endl;
	}
	//遍历指定路径下文件夹中文件名称、大小、创建时间等
	try
	{
		do
		{
			cout << "文件大小为 " << fileInfo.size << "文件名为 " << fileInfo.name << endl;
			string picname = fileInfo.name;
			files.push_back(picname);
			//将图片名称以"#"分割。并存入容器中
			//文件的名字格式：GNXM@001#68359998#2021-01-17 18+10+50#cap.jpg
			//解析后的数据为"GNXM@001", "68359998", "2021-01-17 18+10+50" , "cap.jpg"
			rfid_record = splitstr(picname, delim);
			rfid_record[2] = replace_all(rfid_record[2], "+", ":"); //将时间中的"+"替换为":"
			rfid_record[3] = rfid_record[3].replace(rfid_record[3].find("."), 4, "");//去掉.jpg，只保留照片类型（cap/blank）
			//cout << rfid_record[2] << endl;
			//cout << rfid_record[3] << rfid_record[3].size()<< endl;
			if (fileInfo.size < 35000 && rfid_record[3] == "cap")//是抓图且文件小于一定大小，说明不完整,另一个线程正在保存文件
			{
				Sleep(5 * 1000);
			}
			else  //不是抓图或者抓图大于一定大小，说明抓图保存完成
			{
				//图片转base64编码
				string base64_data = pic_convertto_bin((path + fileInfo.name).c_str());
				//cout << base64_data << endl;


				json upload_data;
				upload_data["address"] = rfid_record[0];
				upload_data["cardNo"] = rfid_record[1];
				upload_data["picture"] = base64_data;
				upload_data["time"] = rfid_record[2];
				//cout << upload_data << endl;
				
				string s = upload_data.dump();
				cout << s << endl;
				
				string url_upload = "http://110.17.165.146:7080/GuangNaReceived/counter/add";
				cpr::Payload payload = cpr::Payload{ {"address",rfid_record[0]},{"cardNo",rfid_record[1]},
					{"picture",base64_data},{"time",rfid_record[2]} };
				cpr::Body body = cpr::Body( s );
				cpr::Response r = cpr::Post(cpr::Url(url_upload),
					cpr::Header{ {"Content-Type","application/json"} },body );

					//cpr::Payload{ {"address",""} });
					//cpr::Body{"test"});
					/*
					cpr::Payload{ {"address",rfid_record[0]},
					{"cardNo",rfid_record[1]},
					{"picture",base64_data},
					{"time",rfid_record[2]}});
					*/
					

				cout << r.status_code << endl;
				cout << r.header["content-type"] << endl;
				
				auto post_response = json::parse(r.text);
				cout << post_response << endl;
				cout << post_response["result"]["success"] << endl;
				break;
			}


		} while (_findnext64(handle, &fileInfo) != -1);
		cout << "查找到" << files.size() << "个文件" << endl;
	}


	catch (std::exception e)
	{
		std::string s = e.what();
		cout << s << endl;
	}
	
}

void test_json()
{
	json upload_data;
	upload_data["address"] = "GNXM@001";
	upload_data["cardNo"] = "68359998";
	upload_data["picture"] = "data:image/jpg;base64,";
	upload_data["time"] = "2020-07-14 16:45:54";
	cout << upload_data << endl;
}


int main() {

	//string s ="b'\x11\x00\xee\x00\xe0 \x820b6\xaa \x04\x13\x13\xfa\xc4\\'";
	//string s = "c45c1100e";
	//cout << "字符串长度为： " << s.length() << endl;
	//s.erase(s.length() - 1, 1);
	//s.erase(0, 2);
	
	//cout << s << endl;


	//get_rfid_confg_from_request();
	
	/*
	queue<vector<string>> picinfo;
	queue<Mat> pic;
	//传引用，需要用ref()进行包装
	thread th1 = thread(rev_serial,std::ref(picinfo),ref(pic));
	thread th2 = thread(sav_data, std::ref(picinfo), ref(pic));
	
	th1.join();
	th2.join();
	*/
	
	uplode_data();

	//const char* filename = "C:\\Users\\Hasee\\Desktop\\testPic\\GNXM@001#68359998#2021-01-17 18+26+00#cap.jpg";
	//pic_convertto_bin(filename);

	//test_json();


	
	return 0;
}