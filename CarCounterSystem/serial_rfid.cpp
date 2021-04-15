#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
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

//���ݻ�ȡ��rfid�����������ݣ����±���rfid_carNum�����ļ�
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

//�ӱ����ļ��ж�ȡrfid�������ñ�����map
map<string,string> read_rfid_carNum_file()
{
	string filename = "rfid_carNum.xlsx";
	//�鿴�ļ��Ƿ����
	if (_access(filename.c_str(), 00) == -1)
	{
		//�ļ�������
		cout << "�ļ�������" << endl;
	}
	//�鿴�ļ��Ƿ�ɶ�
	while (_access(filename.c_str(), 2) != 0)
	{
		cout << "�ļ����ɶ����ȴ�" << endl;
		Sleep(2 * 1000);
	}

	xlnt::workbook wb;
	wb.load(filename);
	auto ws = wb.active_sheet(); //��ȡ��ǰ��Ծ��sheet
	int max_row = ws.highest_row();
	map<string, string> dic;
	int row_num = 0;
	int column_num = 1;
	//����ÿһ��
	for (auto row : ws.rows(false))
	{
		row_num++;
		string rfid;
		string carNum;
		//��rfid�ͳ���ֵ����dic��
		for (auto cell : row)
		{
			//�ӱ��ڶ��п�ʼ��ȡ����
			if (row_num == 1) continue;
			if (column_num == 1)
			{
				rfid = cell.to_string();
				column_num++;
			}
			else
			{
				carNum = cell.to_string();
				column_num--;
			}
		}
		if(row_num!=1) dic[rfid] = carNum; //��һ�����ݲ���
		
	}
	return dic;
}

//��server�����л�ȡrfid�ͳ��Ŷ�Ӧ��
void get_rfid_confg_from_request() {
	int request_times = 0;
	map<string, string> dic_rfid_carNum;
	while (request_times < 5)
	{
		//����http�����Ƿ���Ч�õ���վhttps://www.httpbin.org/get
		cpr::Response r = cpr::Get(cpr::Url{ "http://110.17.165.146:7080/GuangNaReceived/counter/initConfig?address=GNXM" });
		//��ȡdata�����е��ַ�����ת��Ϊjson��ʽ
		string s = r.text;
		cout << "���յ���json����Ϊ��" << s << endl;
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
	// ������ʱ�ļ�����ʾ����������
	FILE* fp;
	fp = fopen("updated", "a");//Ϊ������ļ�����������������ļ�ĩβ׷�����ݣ�����ļ������ڣ��������ļ���
	fclose(fp);
	


	//string config_json = r.dump();
	//cout << config_json;
}

//��������������
void clear_times() {
    //������ʱ�ļ������ڱ�ʶ�Ƿ���Ҫ��ճ�������������¼
	FILE* fp;
	fp = fopen("clear", "a");//a ���ļ��������򴴽����ļ�
	fclose(fp);
	cout << "�Ѵ���clear�ļ�" << endl;
}

//ÿ���Զ����³�����Ϣ�������������
void update_rfid_confg() {
	while (true)
	{
		Sleep(3 * 1000);
		time_t now = time(0);
		//cout << "1970 ��Ŀǰ��������:" << now << endl;
		//תΪtm�ṹ��
		tm* ltm = localtime(&now);
		//nowtimeΪ��ǰСʱ����
		string nowtime = to_string(ltm->tm_hour) + ":" + to_string(ltm->tm_min);
		cout << nowtime << endl;
		if (nowtime == "05:50") get_rfid_confg_from_request();
		else if (nowtime == "06:00") clear_times();//�װ������������
		else if (nowtime == "21:31") clear_times();//ҹ�������������
	}

	/*
	��� tm �ṹ�ĸ�����ɲ���
	cout << "��: " << 1900 + ltm->tm_year << endl;
	cout << "��: " << 1 + ltm->tm_mon << endl;
	cout << "��: " << ltm->tm_mday << endl;
	cout << "ʱ��: " << ltm->tm_hour << ":";
	cout << ltm->tm_min << ":";
	cout << ltm->tm_sec << endl;
	*/ 

}

//��ʼ������ͷ
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

//��ӱ�hex�Զ�ɾȥ��0
string modHexdata(string tmp) {
	if (tmp.length() == 1)
	{
		tmp = "0" + tmp;
		return tmp;
	}
	else
		return tmp;
}


//���մ������ݲ�������ͷ������Ƭ
void rev_serial(queue<vector<string>> &picinfo,queue<Mat> &pic) {
	WzSerialPort serialport;
	while (true)
	{
		bool is_open = serialport.open("COM4", 57600, 0, 8, 1);
		if (is_open) {
			cout << "�����Ѵ�" << endl;
			break;
		}
	}
	VideoCapture cap;
	//ini_cap(cap);
	map<string, unsigned int> last_rfid;//�洢���µ�rfid��ʱ��
	map<string, string> dic_rfid_carNum=read_rfid_carNum_file();//�������ñ�
	map<string, int> dic_carNum_times;//����������Ӧ��¼
	while (true)
	{
		//�ж��Ƿ���Ҫ������������
		if (_access("updated", 00) != -1)  //00����ֻ����ļ��Ƿ���ڣ�����ļ����ڣ�����0�����򷵻�-1
		{
			dic_rfid_carNum = read_rfid_carNum_file();//��ʼ������ȡ�������ñ�

			for (map<string, string>::iterator iter1 = dic_rfid_carNum.begin(); iter1 != dic_rfid_carNum.end(); iter1++)
			{
				bool flag = false; //������¼����������ֵ�����Ƿ����г���
				//˫��ѭ����ʼ������������ֵ��
				for (map<string, int>::iterator iter2 = dic_carNum_times.begin(); iter2 != dic_carNum_times.end(); iter2++)
				{

					//������г���������¼���������ѭ����
					if (iter1->second == iter2->first)
					{
						flag = true;
						break;
					}
				}
				//���µĳ��ţ�����г�ʼ��
				//ֱ�������鷽ʽ��ֵmap����ݼ���ֵ�Զ�����
				if (flag == false) dic_carNum_times[iter1->second] = 0;
			}
			remove("updated");
			cout << "�������" << endl;
		}

		//�ж��Ƿ���Ҫ�������
		if (_access("clear", 00) != -1) //������clear�ļ�
		{
			for (map<string, int>::iterator iter1 = dic_carNum_times.begin(); iter1 != dic_carNum_times.end(); iter1++)
			{
				dic_carNum_times[iter1->first] = 0;
			}
			cout << "������������" << endl;
			remove("clear");
		}

		unsigned char buf[1024];
		string hex_data = "";
		string tmp;
		stringstream ss;
		time_t now = time(0);//ȡ�ô�1970��1��1�������������
		unsigned int now_time = unsigned int(now);
		cout << now_time << endl;
		try
		{
			memset(buf, 0, 1024);
			hex_data.clear();
			tmp.clear();

			Sleep(1 * 1000);

			//serialport.receive(buf, 1024);
			//cout << "test" << endl;
			int len = serialport.receive(buf, 1024);
			cout << "WZSerialPort���յ������ݳ���Ϊ: " << len << endl;
			//����ַ�ת16����д���ַ���hex_data��
			for (int i = 0; i < len; i++)
			{
				ss.clear();
				ss << hex << (int)buf[i];
				ss >> tmp;
				//ת��16���Ƶ����ݿ�����Ϊ��λΪ0����ʡ�ԣ���modHexdata()����ӱ�ʡ�Ե�0��
				hex_data = hex_data + modHexdata(tmp);
			}


			cout << "�������16�����ַ�������Ϊ��" << hex_data.length() << endl;
			cout << "�������16��������Ϊ��" << hex_data << endl;

			string head_rfid = hex_data.substr(0, 6);
			if (head_rfid == "1100ee" && hex_data.length() >= 36)//1100ee˵���õ�������Ϊ�������ݵ����������п���һ�������ж��rfid����,����Ϊ��ȷ�Ŀ�ʼ֡
			{
				bool is_Newdata = false;
				bool is_Exist_rfid = false;
				//��16��������תΪ10����rfid����
				int rfidNum = stoi(hex_data.substr(24, 8), nullptr, 16);
				cout << "��������rfid����Ϊ��" << rfidNum << endl;
				for (map<string, unsigned int>::iterator iter = last_rfid.begin(); iter != last_rfid.end(); iter++)
				{
					//cout << "1111" << endl;
					//��������������¼����,��ʱ��������300�룬��Ϊ�����ļ�¼��
					if (to_string(rfidNum) == iter->first)
					{
						is_Exist_rfid = true;
						if ((now_time - iter->second) > 30) {
							is_Newdata = true;
							break;
						}
					}
					else is_Exist_rfid = false;
				}
				cout << "is_Exist_rfid=" << is_Exist_rfid << endl;
				cout << "is_Newdata=" << is_Newdata << endl;
				//����޼�¼���ж�Ϊ�����ݣ���������������и��ġ�
				if (!is_Exist_rfid || is_Newdata)
				{
					bool is_Exist_car = false;
					for (map<string, string>::iterator iter = dic_rfid_carNum.begin(); iter != dic_rfid_carNum.end(); iter++)
					{
						if (to_string(rfidNum) == iter->first)
						{
							is_Exist_car = true;
							break;
						}
					}
					if (is_Exist_car)
					{
						string current_car_num = dic_rfid_carNum[to_string(rfidNum)];
						cout << current_car_num << endl;
						//��Ӧ������������+1
						dic_carNum_times[current_car_num] = dic_carNum_times[current_car_num] + 1;
						cout << dic_carNum_times[current_car_num] << endl;
					}
					else
						cout << "car missing" << endl;

					//ץͼ,��ʼ��
					ini_cap(cap);
					double width = cap.get(CV_CAP_PROP_FRAME_WIDTH);
					double height = cap.get(CV_CAP_PROP_FRAME_HEIGHT);
					cout << "��" << width << " ��" << height << endl;

					int times = 0;

					string flag;
					string cap_time;
					//string now_time = "��ûд";
					vector<string> record;
					Mat frame;
					Mat blankframe(480, 640, CV_8UC1, Scalar(255));

					while (times < 5)
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
						cap_time = GetSystemTime();
						flag = "blank";
						frame = blankframe;
					}
					cap.release();

					//string rfidNum = "68359998";

					record.push_back(to_string(rfidNum));
					record.push_back(to_string(now_time));
					record.push_back(cap_time);
					record.push_back(flag);

					picinfo.push(record);



					pic.push(frame);
					last_rfid[to_string(rfidNum)] = now_time;
				}
			}
		}

		catch (std::exception e)
		{
			std::string s = e.what();
			cout << s << endl;
		}
	}
	
	/*
	for (map<string, string>::iterator iter1 = dic_rfid_carNum.begin(); iter1 != dic_rfid_carNum.end(); iter1++)
	{
		cout << iter1->first << " " << iter1->second << endl;
	}

	for (map<string, int>::iterator iter2 = dic_carNum_times.begin(); iter2 != dic_carNum_times.end(); iter2++)
	{
		cout<< iter2->first << " " << iter2->second << endl;
	}
	*/
	
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
	CreateDirectoryA("savePic", 0);
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
		imwrite("savePic/" + pic_name +".jpg", frame, compression_params);
		/*
		imshow("����", frame);
		waitKey(0);
		*/
		
	}
}

//�ָ��ַ���
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

//�ַ����滻ȫ��ĳһָ���ַ�
string replace_all(string str, string preStr, string nextStr)
{
	string::size_type startpos = 0;
	while (startpos != string::npos) //���û�ҵ�������һ���ر�ı�־c++����npos��ʾ
	{
		startpos = str.find(preStr);
		//cout << startpos << endl;
		//�滻
		if (startpos != string::npos)
		{
			str.replace(startpos, 1, nextStr);
		}
	}
	return str;
}

string pic_convertto_bin(const char* Filename) {
	//�Զ����Ʒ�ʽ���ļ������ļ��������뵽�ڴ�
	ifstream file(Filename, ifstream::in | ifstream::binary);
	file.seekg(0, file.end); //ƫ����0�����ļ�ĩβ��ʼƫ��
	int file_length = file.tellg();
	cout << file_length << endl;

	file.seekg(0, file.beg);
	char* buffer = new char[file_length];
	file.read(buffer, file_length);
	
	string base64_data = base64_encode(reinterpret_cast<const unsigned char*>(buffer), file_length);
	base64_data = "data:image/jpg;base64," + base64_data;
	//cout << "base64���볤��Ϊ�� "<<base64_data.length()<<endl<< base64_data << endl;
	//ɾ��buffer,���ָ��
	delete[]buffer;
	file.close();
	return base64_data;
	
	
	
	
}

//����ͼƬ�ϴ�������
void uplode_data() {
	CreateDirectoryA("rfid_pic_finish", 0);
	intptr_t handle;
	_finddatai64_t fileInfo;
	vector<string> files;
	vector<string> rfid_record;
	string search_path = "savePic/*.jpg";
	string path = "savePic/";
	const char delim[2] = "#";
	handle = _findfirst64(search_path.c_str(), &fileInfo);
	if (handle==-1)
	{
		cout << "�ļ�����û�ж�ȡ���ļ�" << endl;
	}
	//����ָ��·�����ļ������ļ����ơ���С������ʱ���
	try
	{
		do
		{
			cout << "�ļ���СΪ " << fileInfo.size << "�ļ���Ϊ " << fileInfo.name << endl;
			string picname = fileInfo.name;
			files.push_back(picname);
			//��ͼƬ������"#"�ָ������������
			//�ļ������ָ�ʽ��GNXM@001#68359998#2021-01-17 18+10+50#cap.jpg
			//�����������Ϊ"GNXM@001", "68359998", "2021-01-17 18+10+50" , "cap.jpg"
			rfid_record = splitstr(picname, delim);
			rfid_record[2] = replace_all(rfid_record[2], "+", ":"); //��ʱ���е�"+"�滻Ϊ":"
			rfid_record[3] = rfid_record[3].replace(rfid_record[3].find("."), 4, "");//ȥ��.jpg��ֻ������Ƭ���ͣ�cap/blank��
			//cout << rfid_record[2] << endl;
			//cout << rfid_record[3] << rfid_record[3].size()<< endl;
			if (fileInfo.size < 35000 && rfid_record[3] == "cap")//��ץͼ���ļ�С��һ����С��˵��������,��һ���߳����ڱ����ļ�
			{
				Sleep(3 * 1000);
			}
			else  //����ץͼ����ץͼ����һ����С��˵��ץͼ�������
			{
				
				//ͼƬתbase64����
				string base64_data = pic_convertto_bin((path + fileInfo.name).c_str());
				cout << 111 << endl;
				//cout << base64_data << endl;


				json upload_data;
				upload_data["address"] = rfid_record[0];
				upload_data["cardNo"] = rfid_record[1];
				upload_data["picture"] = base64_data;
				upload_data["time"] = rfid_record[2];
				//cout << upload_data << endl;
				
				//��jsonתΪ�ַ���
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
				//�ϴ��ɹ�����ͼƬת�Ƶ�rfid_pic_finish�ļ���
				if (post_response["result"]["success"] = true)
				{
					string prefile = "savePic/" + string(fileInfo.name);
					string nextfile = "rfid_pic_finish/"+string(fileInfo.name);
					MoveFileA(prefile.c_str(), nextfile.c_str());
				}
				break;
			}

		} while (_findnext64(handle, &fileInfo) != -1);
		cout << "���ҵ�" << files.size() << "���ļ�" << endl;
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

	//get_rfid_confg_from_request();


	/*
		
	queue<vector<string>> picinfo;
	queue<Mat> pic;
	//�����ã���Ҫ��ref()���а�װ
	thread th1 = thread(rev_serial, std::ref(picinfo), ref(pic));
	thread th2 = thread(sav_data, std::ref(picinfo), ref(pic));

	th1.join();
	th2.join();

	*/


	//uplode_data();

	//read_rfid_carNum_file();

	//update_rfid_confg();



	return 0;
}