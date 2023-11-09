#include <iostream>
#include <fstream>
#include <windows.h>

using namespace std;

int lineCount(fstream&);

int main() {
    
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    int CORE_NUMBER = sysInfo.dwNumberOfProcessors;
    string FILE_NAME = "dataset1";
    fstream FILE;
    int TIME = -1;

    do {
        cout << "Dataset file name?\t";
        cin >> FILE_NAME;
        FILE.open("dataset/" + FILE_NAME + ".txt", ios::in);
        if (!FILE){
            cout << "File not found, try again!" << endl;
        }
    } while (!FILE);

    do {
        cout << "Time in seconds?\t";
        cin >> TIME;
        if (TIME <= 0){
            cout << "Invalid time, try again!" << endl;
        }
    } while (TIME <= 0);


    int DATA_COUNT = lineCount(FILE);

    float* dataset = new float[DATA_COUNT];

    cout << "Working on " << FILE_NAME << " with " << DATA_COUNT << " records for " << TIME << " seconds:" << endl;

    for (int i = 0; i < DATA_COUNT; i++){
        string read;
        getline(FILE, read, '\n');
        dataset[i] = stof(read);
        cout << dataset[i] << ' ';
    }

    cout << endl <<  CORE_NUMBER << " CPU cores detected, launching " << CORE_NUMBER << " child processes" << endl;

    STARTUPINFO PS[CORE_NUMBER];
    PROCESS_INFORMATION PP[CORE_NUMBER];

    HANDLE baseshm = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,  3 * sizeof(int), TEXT("osprojbase"));

    int* basebuff = (int*) MapViewOfFile(baseshm, FILE_MAP_ALL_ACCESS, 0, 0, 3 * sizeof(int));

    HANDLE mapshm = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,  CORE_NUMBER * sizeof(int), TEXT("osprojmap"));

    int* mapbuff = (int*) MapViewOfFile(mapshm, FILE_MAP_ALL_ACCESS, 0, 0, CORE_NUMBER * sizeof(int));

    HANDLE datasetshm = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,  DATA_COUNT * sizeof(float), TEXT("osprojdataset"));

    float* datasetbuff = (float*) MapViewOfFile(datasetshm, FILE_MAP_ALL_ACCESS, 0, 0, DATA_COUNT * sizeof(float));

    HANDLE optshm = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,  3 * sizeof(float), TEXT("osprojopt"));

    float* optbuff = (float*) MapViewOfFile(optshm, FILE_MAP_ALL_ACCESS, 0, 0, 3 * sizeof(float));

    HANDLE answershm = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0,  CORE_NUMBER * 3 * DATA_COUNT * sizeof(float), TEXT("osprojans"));

    float* answerbuff = (float*) MapViewOfFile(answershm, FILE_MAP_ALL_ACCESS, 0, 0, CORE_NUMBER * 3 * DATA_COUNT * sizeof(float));

    basebuff[0] = CORE_NUMBER;
    basebuff[1] = DATA_COUNT;
    basebuff[2] = TIME;

    float total = 0;

    for (int i = 0; i < DATA_COUNT; i++){
        datasetbuff[i] = dataset[i];
        total += dataset[i];
    }

    optbuff[0] = total / 5 * 2;
    optbuff[1] = total / 5 * 2;
    optbuff[2] = total / 5;

    for (int i = 0; i < CORE_NUMBER * 3 * DATA_COUNT; i++){
        answerbuff[i] = 0;
    }

    for (int i = 0; i < CORE_NUMBER; i++){
        ZeroMemory(&PS[i], sizeof(PS[i]));
        PS[0].cb = sizeof(PS[i]);
        ZeroMemory(&PP[i], sizeof(PP[i]));
        bool pc = CreateProcess(TEXT("child.exe"), NULL, NULL, NULL, FALSE, 0, NULL, NULL, &PS[i], &PP[i]);
        if (!pc){
            cout << "Creating process failed!" << endl;
            return 0;
        }
	    int pid = GetProcessId(PP[i].hProcess);
	    mapbuff[i] = pid;
    }

    for (int i = 0; i < CORE_NUMBER; i++){
    	WaitForSingleObject(PP[i].hProcess, INFINITE);
    	CloseHandle(PP[i].hProcess);
    	CloseHandle(PP[i].hThread);
	}

    string keyword[] = {"First", "Second", "Third"};

    int winnerIndex = -1;
    float lastDiff = -1;

    for (int i = 0; i < CORE_NUMBER; i++){
        // cout << "PID = " << mapbuff[i] << endl;
        float diff = 0;
        for (int j = 0; j < 3; j++){
            float childSum = 0;
            // cout << keyword[j] << " CHILD : " << endl;
            for (int k = 0; k < DATA_COUNT; k++){
                // cout << answerbuff[(i * 3 * DATA_COUNT) + (j * DATA_COUNT) + k] << " ";
                childSum += answerbuff[(i * 3 * DATA_COUNT) + (j * DATA_COUNT) + k];
            }
            // cout << endl;
            if (j == 0 || j == 1){
                diff += abs(childSum - total / 5 * 2);
            } else if (j == 2){
                diff += abs(childSum - total / 5);
            }
        }  
        // cout << "-------------------------------" << endl;
        if (diff < lastDiff || lastDiff == -1){
            winnerIndex = i;
            lastDiff = diff;
        }
    }

    cout << "Winner PID is " << mapbuff[winnerIndex] << " with an offset of " << lastDiff << endl;

    for (int i = 0; i < 3; i++){
        cout << keyword[i] << " CHILD : " << endl;
        for (int j = 0; j < DATA_COUNT; j++){
            cout << answerbuff[(winnerIndex * 3 * DATA_COUNT) + (i * DATA_COUNT) + j] << " ";
        }
        cout << endl;
    }

}

int lineCount(fstream& F){

    int res = 0;
    string read;
    while (getline(F, read)){
        res++;
    };
    F.clear();
    F.seekg(0, ios::beg);
    return res;

}
