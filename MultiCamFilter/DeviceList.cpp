#include <streams.h>
#include <stdio.h>
#include <iostream>
#include <fstream>
using namespace std;

//#include <strsafe.h>
#include <string>
#include <vector>
#include <set>
#include <map>
//#include <Dvdmedia.h>

#include "jmac-vcam-guids.h"
#include "LogHelpers.h"
#include "DeviceHelpers.h"
#include "DeviceList.h"


const char* DeviceList::s_DIRNAME = "c:\\temp";
const char* DeviceList::s_FILENAME = "c:\\temp\\multicam-devices.txt";
const char* DeviceList::s_COMMENTSTART = "#";
#define MAX_DEVICE_LEN 1024

DeviceList::DeviceList(void) :
	deviceListExists(FALSE)
{
}


DeviceList::~DeviceList(void)
{
}

BOOL DeviceList::OpenDeviceFile()
{
	BOOL openedOK = TRUE;
	try {
		deviceFile.exceptions(ios::failbit);
		deviceFile.open(s_FILENAME);
	}
	catch (...)
	{
		openedOK = FALSE;
	}
	return openedOK;
}


void DeviceList::ReadList()
{
	BOOL openedOK = OpenDeviceFile();
	if (!openedOK)
	{
		vcamLog(10, "DeviceList::ReadList: No device file found");
		goto cleanup;
	}
	else
	{
		vcamLog(10, "DeviceList::ReadList: Found device file");
	}

	char line[MAX_DEVICE_LEN];
	BOOL done = FALSE;
	while (!done) {
		BOOL readOK = TRUE;
		try {
			deviceFile.getline(line, MAX_DEVICE_LEN);
		}
		catch (...)
		{
			readOK = FALSE;
		}
		if (!readOK) {
			done = TRUE;
			break;
		}
		string deviceString(line);
		if (deviceString.find(s_COMMENTSTART) == 0)
		{
			continue;			
		}
		int trimPos = deviceString.find_last_not_of(" \n\r\t");
		if (trimPos != string::npos) {
			deviceString.erase(trimPos+1); 
		}
		else {
			continue;			
		}
		if (deviceString.length() == 0)
		{
			continue;			
		}
		//devicesInFile.insert(
		//vector<char> defaultDevice;
		//defaultDevice.push_back('1');
		//devicesInFile[deviceString] = defaultDevice;

		devicesInFile.insert(deviceString);
		vcamLog(50, "DeviceList::ReadList: read device '%s'", deviceString.c_str());
	}

cleanup:
	if (openedOK)
	{
		deviceFile.close();
	}

	if (devicesInFile.size() == 0) 
	{
		deviceListExists = FALSE;
	}
	else
	{
		deviceListExists = TRUE;
	}
}

BOOL DeviceList::ContainsDevice(string deviceName)
{
	return (devicesInFile.count(deviceName) == 1);
	//return TRUE;
}
