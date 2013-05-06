#pragma once

#include <iostream>
#include <set>
#include <string>

using namespace std;


class DeviceList
{
public:
	DeviceList(void);
	virtual ~DeviceList(void);
	void DeviceList::ReadList();
	BOOL DeviceListExists() {return deviceListExists;};
	BOOL ContainsDevice(string deviceName);

protected:
	BOOL OpenDeviceFile();

	static const char* s_DIRNAME;
	static const char* s_FILENAME;
	static const char* s_COMMENTSTART;
	ifstream deviceFile;
	
	//set<string> devicesInFile;
	//vector<string> devicesInFile2;
	set<string> devicesInFile;
	//map<string, vector<char>> devicesInFile3;
	BOOL deviceListExists;


};

