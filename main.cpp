#include <stdio.h>
#include <tchar.h>
#include "SerialClass.h"	// Library described above
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
using namespace std;

// application reads from the specified serial port and reports the collected data
int _tmain(int argc, _TCHAR* argv[])
{
	char input[10], str[512];
	cout<<"Welcome to the serial downloader!\nCreated by Andrei-Paul Tocu at Middlesex University, Feb 2022\n"<<endl;
	
	Serial* SP;
	while(SP->IsConnected() == 0){
		cout<<"Please type your COM port number. e.g. 'COM1', 'COM5', 'COM10'"<<endl<<"If unsure write '?'"<<endl;
		cin>>input;
		if(strstr(input,"?")){
			printf("Scanning ports ...\n");
			system("wmic path Win32_SerialPort > temp.txt");
			
			ifstream tempfile;
			tempfile.open("temp.txt", std::fstream::in | std::fstream::out | std::fstream::app);
			
			string fileToString;
			size_t found=0;
			cout<<"Found ports ";
			while(getline(tempfile, fileToString)){
				fileToString.erase(remove(fileToString.begin(), fileToString.end(), ' '), fileToString.end());
				found = fileToString.find('(');
				if(found!=string::npos) cout<<fileToString.substr(found+1,12)<<", ";
			}
			cout<<endl;
			tempfile.close();
			if(remove("temp.txt")) printf("\nCannot delete temp file!\n");
			
		}else{
			strcpy(str,"\\\\.\\");
			strcat(str,input);
			strcpy(input,str);
			//printf("Received port: %s\n", input);

			//Serial* SP = new Serial("\\\\.\\COM10");    // adjust as needed
			SP = new Serial(input);    // adjust as needed
		}
	}
	
	strcpy(str,"download\n");
	if (SP->IsConnected()){
		printf("We're connected!\n");
		if(SP->WriteData(str, strlen(str))) printf("Download request sent, now waiting for the data to be received!\nMake sure your board is in the DISARMED state - GREEN LED breathing\n");
	}

	char incomingData[256] = "";			// don't forget to pre-allocate memory
	int dataLength = 255;
	int readResult = 0;
	ofstream myfile;
	myfile.open ("serialdownload.txt", std::fstream::in | std::fstream::out | std::fstream::app);
	char * disconnect;
	while(SP->IsConnected())
	{
		readResult = SP->ReadData(incomingData,dataLength);
		// printf("Bytes read: (0 means no data available) %i\n",readResult);
		incomingData[readResult] = 0;
       	printf("%s",incomingData);
       	disconnect = strstr(incomingData, "##### END OF DOWNLOAD #####");
		myfile << incomingData;
		Sleep(100);
		if(disconnect != NULL){
			myfile.close();
			printf("Received data is now stored locally inside file serialdownload.txt\nGoodbye!");
			break;
		}
	}
	return 0;
}
