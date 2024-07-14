#pragma once

#define BINARY_THRESHOLD 50
#include <vector>

extern int BCArunning;	// -1 running backward
						// 0 stopped
						// +1 runing forward
extern int CurrentIteration; // also used to tell whether current frame is even or odd
extern BOOL EvenStep;	// True when iteration step is even
extern BOOL BCAimageLoaded; // TRUE is image is loaded into memory for processing
extern BOOL StopRunning; // request to stop iterations at the end of the  current iteration

extern int* TheImage;
extern IMAGINGHEADER BCAimageHeader;
extern int ForwardRules[16];
extern int BackwardRules[16];

int ReadRulesFile(HWND hDlg, WCHAR* InputFile, int* Rules);
void MargolusBCAp1p1(BOOL EvenStep, int* TheImage, int Xsize, int Ysize,
	int* Rules, int* Histo);
int ReadASISmessage(WCHAR* Filename, IMAGINGHEADER* ImageHeader, int** NewImage,
	BYTE* Header, BYTE* Footer, int* BCAiterations, int* BitCount);
int BitSequences(BYTE* BitList, int* BitCountList, int MaxSequence, BOOL BitOrder);
int ConvertImage2Bitstream(int* InputImage, IMAGINGHEADER* ImageHeader, 
	BYTE** MessageBody, int MessageLength,int* BitCount);
void CollapseImageFrames(int* Image, IMAGINGHEADER* ImageHeader, int Threshold);
void BinarizeImage(int* TheImage, IMAGINGHEADER* BCAimageHeader, int Threshold);
int CountBitInImage(int* Image, IMAGINGHEADER* ImageHeader);
void findFactors(int num, int start, std::vector<int>& current, std::vector<std::vector<int>>& result);
std::vector<std::vector<int>> factorCombinations(int num);