#pragma once
// 
// function prototypes
//
BOOL CCFileSave(HWND hWnd, LPWSTR pszCurrentFilename, LPWSTR* pszFilename,
BOOL bSelectFolder, int NumTypes, COMDLG_FILTERSPEC* FileTypes, LPCWSTR szDefExt);
BOOL CCFileOpen(HWND hWnd, LPWSTR pszCurrentFilename, LPWSTR* pszFilename,
BOOL bSelectFolder, int NumTypes, COMDLG_FILTERSPEC* FileTypes, LPCWSTR szDefExt);
int ReadImageHeader(WCHAR* Filename, IMAGINGHEADER* ImageHeader);
int LoadImageFile(int** ImagePtr, WCHAR* ImagingFilename, IMAGINGHEADER* Header);
int SaveImageFile(HWND hDlg, int* TheImage, WCHAR* Filename, IMAGINGHEADER* Header);
int ReadBMPfile(int** ImagePtr, WCHAR* InputFilename, IMAGINGHEADER* ImgHeader);
int SaveBMP(WCHAR* Filename, WCHAR* InputFile, int RGBframes, int AutoScale);
int SaveTXT(WCHAR* Filename, WCHAR* InputFile);
int HEX2Binary(HWND hWnd);
int CamIRaImport(HWND hWnd);
int GetFileSize(WCHAR* szString);
int SaveBMP2PNG(WCHAR* Filename);
int SaveImageBMP(WCHAR* Filename, COLORREF* Image, int ImageXextent, int ImageYextent);
int SaveBYTEs2Text(WCHAR* OutputFile, BYTE* ByteStream,
    int NumBytes, int BitOrder);
int ReadBYTEs2Text(WCHAR* InputFile, BYTE* ByteStream,
    int NumBytes, int BitOrder);
int SaveASISbitstream(WCHAR* Filename, BYTE* Header, BYTE* MessageBody, BYTE* Footer);
int SaveHistogramData(WCHAR* Filename, BOOL CreateNew, int Index, int* Histogram, int NumEntries);
int SaveSnapshot(HWND hDlg, int CurrentIteration, int* TheImage, IMAGINGHEADER* BCAimageHeader);
