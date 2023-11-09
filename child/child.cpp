#include <iostream>
#include <chrono>
#include <stdio.h>
#include <time.h>
#include <windows.h>

using namespace std;

int getIndex(int map[], int size, int pid);
float sumArr(float[], int);

int main() {

    HANDLE baseshm = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, TEXT("osprojbase"));
    int* basebuff = (int*) MapViewOfFile(baseshm, FILE_MAP_READ, 0, 0, 3 * sizeof(int));

    HANDLE datasetshm = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, TEXT("osprojdataset"));
    float* datasetbuff = (float*) MapViewOfFile(datasetshm, FILE_MAP_READ, 0, 0, basebuff[1] * sizeof(float));

    HANDLE mapshm = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, TEXT("osprojmap"));
    int* mapbuff = (int*) MapViewOfFile(mapshm, FILE_MAP_READ, 0, 0, basebuff[0] * sizeof(int));

    HANDLE answershm = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, TEXT("osprojans"));
    float* answerbuff = (float*) MapViewOfFile(answershm, FILE_MAP_ALL_ACCESS, 0, 0, basebuff[0] * 3 * basebuff[1] * sizeof(float));

    HANDLE optshm = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, TEXT("osprojopt"));
    float* optbuff = (float*) MapViewOfFile(optshm, FILE_MAP_READ, 0, 0, 3 * sizeof(float));

    int pid = GetCurrentProcessId();
    // cout << "I AM CHILD WITH PID " << pid << endl;
    int index = -1;

    do {
        index = getIndex(mapbuff, basebuff[0], pid);
        if (index == -1) {
            cout << "couldn't find index" << endl;
        }
    } while (index == -1);

    std::chrono::time_point startTime = std::chrono::high_resolution_clock::now();
    std::chrono::time_point currentTime = startTime;

    srand(time(0) + pid);

    float lastDiff = -1;
    float c1[basebuff[1]] = {0};
    float c2[basebuff[1]] = {0};
    float c3[basebuff[1]] = {0};

    while (std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime).count() < basebuff[2]){
        
        float newC1[basebuff[1]] = {0};
        float newC2[basebuff[1]] = {0};
        float newC3[basebuff[1]] = {0};
        int newC1Cnt = 0, newC2Cnt = 0, newC3Cnt = 0;

        for (int i = 0; i < basebuff[1]; i++){
                        
            int turn = rand() % 3;

            if (turn == 0) {
                newC1[newC1Cnt] = datasetbuff[i];
                newC1Cnt++;
            } else if (turn == 1) {
                newC2[newC2Cnt] = datasetbuff[i];
                newC2Cnt++;
            } else {
                newC3[newC3Cnt] = datasetbuff[i];
                newC3Cnt++;
            }

        }

        float newDiff = abs(sumArr(newC1, basebuff[1]) - optbuff[0]) + abs(sumArr(newC2, basebuff[1]) - optbuff[1]) + abs(sumArr(newC3, basebuff[1]) - optbuff[2]);

        if (lastDiff == -1 || newDiff < lastDiff) {
            lastDiff = newDiff;
            for (int j = 0; j < basebuff[1]; j++){
                c1[j] = newC1[j]; 
                c2[j] = newC2[j]; 
                c3[j] = newC3[j]; 
            }
        }

        currentTime = std::chrono::high_resolution_clock::now();
    }

    for (int i = 0; i < basebuff[1]; i++){
        answerbuff[(index * 3 * basebuff[1]) + (0 * basebuff[1]) + i] = c1[i];
        answerbuff[(index * 3 * basebuff[1]) + (1 * basebuff[1]) + i] = c2[i];
        answerbuff[(index * 3 * basebuff[1]) + (2 * basebuff[1]) + i] = c3[i];
    }

    return 0;

}

int getIndex(int map[], int size, int pid){

    for (int i = 0; i < size; i++){
        if (map[i] == pid){
            return i;
        }
    }

    return -1;

}

float sumArr(float arr[], int size) {
    
    float res = 0;
    for (int i = 0; i < size; i++){
        res += arr[i];
    }

    return res;

}
