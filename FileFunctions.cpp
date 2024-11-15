//
// MySETIBCA, an application for decoding, encoding message images using 
// a block cellular automata like what was used in the 'A Sign inSpace' project message
// 
// FileFunctions.cpp
// (C) 2023, Mark Stegall
// Author: Mark Stegall
//
// This file is part of MySETIBCA.
//
// MySETIBCA is free software : you can redistribute it and /or modify it under
// the terms of the GNU General Public License as published by the Free Software Foundation,
// either version 3 of the License, or (at your option) any later version.
//
// MySETIBCA is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
// without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
// See the GNU General Public License for more details.
// You should have received a copy of the GNU General Public License along with MySETIBCA.
// If not, see < https://www.gnu.org/licenses/>.//
// 
// 
// V1.0.0	2024-06-21	Initial release
// V1.1.0   2024-06-28  Added ASIS receive/send message
//                      support functions added:
//                          SaveBYTEs2Text()
//                          ReadBYTEs2Text()
//                          SaveASISbitstream()
//                      Corrected bug when reading BMP files
//                          ReadBMPfile() is corrected function
//                          deleted LoadBMPfile()
// V1.1.2   2024-07-06  Added SaveHistogramData()
// V1.1.5   2024-11-08  Added SaveSnapshot()
//                      Changed index number to have leading zero, with 8 significant digits
//                          like 00000001,00000002,...
//                          This solves the alphabetical sorting issues for sorts that don't
//                          understand numbers in the their filename
// 
//  This module is a copy of the FileFunctions module used in MySETIviewer and customized
//  for this application
//
//  FileFunctions
//
// Filename functions for gettting the filename for opening and saving
// Saving and restoring the main window size and position from one execution to the next
// Windows Error reporting message dialog
// Getting version info from the Windows 'resource' definitions
//
// These are file open, save and error reporting arebased on the examples given by Microsoft Docs
// https://docs.microsoft.com/en-us/windows/win32/learnwin32/example--the-open-dialog-box
// 
// The Get Verion info function was based on the original post by Mark Ransom on stack overflow:
// https://stackoverflow.com/questions/316626/how-do-i-read-from-a-version-resource-in-visual-c
// It was changed to include more complete information from from the Version resource and resolve
// syntax errors.
//
// Application standardized error numbers for functions that perform transform processes:
//      1 - success
//      0 - parameter or image header problem
//     -1 memory allocation failure
//     -2 open file failure
//     -3 file read failure
//     -4 incorect file type
//     -5 file sizes mismatch
//     -6 not yet implemented
//
// Some function return TRUE/FALSE results
//
#include "framework.h"
#include "resource.h"
#include <shobjidl.h>
#include <winver.h>
#include <vector>
#include <atlstr.h>
#include <gdiplus.h>
#include "globals.h"
#include <strsafe.h>
#include "shellapi.h"
#include "Appfunctions.h"
#include "imageheader.h"
#include "FileFunctions.h"

//****************************************************************
//
// Generic File Open function
// This uses the current style file dialog for the Windows OS version
//
// BOOL CCFileOpen(HWND hWnd, LPWSTR pszCurrentFilename, LPWSTR* pszFilename,
//                 int NumTypes, COMDLG_FILTERSPEC* FilterSpec)
// 
//      LPWSTR  pszCurrentFilename - if not "" then use as a default file selection
//      LPWSTR* pszFilename - return the selected filename (inluding path) if 'Open' selected
//      BOOL    bSelectFolder - TRUE, select only the folder not a file
//      int     NumTypes - 0 none specifed other wise number in list
//      COMDLG_FILTERSPEC* pointer to file types list
//
// example of file type specifier:
// COMDLG_FILTERSPEC rgSpec[] =
//{
//    { L"Image files", L"*.raw" },
//    { L"All Files", L"*.*" },
//};
// 
// return
//      TRUE - 'Open' selected, pszFilename is valid
//      FALSE-  user cancelled file open
// 
//****************************************************************
BOOL CCFileOpen(HWND hWnd, LPWSTR pszCurrentFilename, LPWSTR* pszFilename,
                BOOL bSelectFolder, int NumTypes, COMDLG_FILTERSPEC* FileTypes,
                LPCWSTR szDefExt)
{
BOOL bReturnValue = FALSE;
// Initialize COM
HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
if (SUCCEEDED(hr))
{
    IFileOpenDialog *pFileOpen = NULL;

    // Create the COM FileOpenDialog object.
    hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
        IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

    if (SUCCEEDED(hr))
    {
         if (wcscmp(pszCurrentFilename, L"") != 0) {
            //parse for just filename
            int err;
            WCHAR Drive[_MAX_DRIVE];
            WCHAR Dir[_MAX_DIR];
            WCHAR Fname[_MAX_FNAME];
            WCHAR Ext[_MAX_EXT];
            WCHAR Directory[MAX_PATH];

            // split apart original filename
            err = _wsplitpath_s(pszCurrentFilename, Drive, _MAX_DRIVE, Dir, _MAX_DIR, Fname,
                _MAX_FNAME, Ext, _MAX_EXT);
            if (err != 0) {
                return FALSE;
            }

            err = _wmakepath_s(Directory, _MAX_PATH, Drive, Dir, L"", L"");
            if (err != 0) {
                return FALSE;
            }

            // if no directory try the global CurrentFilename for directory
            if (wcscmp(Directory, L"") == 0) {
                err = _wsplitpath_s(szCurrentFilename, Drive, _MAX_DRIVE, Dir, _MAX_DIR, Fname,
                    _MAX_FNAME, Ext, _MAX_EXT);
                if (err != 0) {
                    return FALSE;
                }

                err = _wmakepath_s(Directory, _MAX_PATH, Drive, Dir, L"", L"");
                if (err != 0) {
                    return FALSE;
                }
            }

            // if still no directory try the global strAppNameEXE for directory
            if (wcscmp(Directory, L"") == 0) {
                err = _wsplitpath_s(strAppNameEXE, Drive, _MAX_DRIVE, Dir, _MAX_DIR, Fname,
                    _MAX_FNAME, Ext, _MAX_EXT);
                if (err != 0) {
                    return FALSE;
                }

                err = _wmakepath_s(Directory, _MAX_PATH, Drive, Dir, L"", L"");
                if (err != 0) {
                    return FALSE;
                }
            }

            IShellItem* pCurFolder = NULL;
            hr = SHCreateItemFromParsingName(Directory, NULL, IID_PPV_ARGS(&pCurFolder));
            if (SUCCEEDED(hr)) {
                hr = pFileOpen->SetFolder(pCurFolder);
                hr = pFileOpen->SetDefaultFolder(pCurFolder);
                pCurFolder->Release();
            }
        }

        if (!bSelectFolder) {
            hr = pFileOpen->SetOptions(FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST);
            if (NumTypes != 0 && FileTypes!=NULL) {
                hr = pFileOpen->SetFileTypes(NumTypes, FileTypes);
            }
            if (szDefExt != NULL) {
                hr = pFileOpen->SetDefaultExtension(szDefExt);
            }

            if (wcscmp(pszCurrentFilename, L"") != 0)
            {
                //parse for just filename
                int err;
                WCHAR Drive[_MAX_DRIVE];
                WCHAR Dir[_MAX_DIR];
                WCHAR Fname[_MAX_FNAME];
                WCHAR Ext[_MAX_EXT];

                // split apart original filename
                err = _wsplitpath_s(pszCurrentFilename, Drive, _MAX_DRIVE, Dir, _MAX_DIR, Fname,
                    _MAX_FNAME, Ext, _MAX_EXT);
                if (err != 0) {
                    return FALSE;
                }
     
                WCHAR NewFname[_MAX_FNAME];

                swprintf_s(NewFname, _MAX_FNAME, L"%s%s", Fname, Ext);
                hr = pFileOpen->SetFileName(NewFname);
            }

        } else {
            hr = pFileOpen->SetOptions(FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST); // This selects only a folder, not a filename
        }

        // Show the Open dialog box.
        hr = pFileOpen->Show(NULL);

        // Get the file name from the dialog box.
        if (SUCCEEDED(hr))
        {
            IShellItem* pItem;
            hr = pFileOpen->GetResult(&pItem);
            if (SUCCEEDED(hr))
            {
                // The allocation for pszFIlename by GetDisplayName must be freed when no longer used
                // using CoTaskMemFree(pszFilename);
                hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, pszFilename);
                pItem->Release();
                bReturnValue = TRUE;
            }
        }
        pFileOpen->Release();
    }
    // release COM
    CoUninitialize();
} else {
    MessageBox(hWnd, (LPCWSTR) L"Failed to create open file resource\nFatal Error", (LPCWSTR)L"Open file dialog", IDOK);
    return FALSE;
}

return bReturnValue;
}

//****************************************************************
//
// Generic File Save function
// This uses the current style file dialog for the Windows OS version
//
// BOOL CCFileSave(HWND hWnd, LPWSTR pszCurrentFilename, LPWSTR* pszFilename)
// 
//      LPWSTR pszCurrentFilename - if not "" then use as a default file selection
//      LPWSTR* pszFilename - return the selected filename (inluding path) if 'Open' selected
//      BOOL    bSelectFolder - TRUE, select only the folder not a file
//      int     NumTypes - 0 none specifed other wise number in list
//      COMDLG_FILTERSPEC* pointer to file types list
//
// example of file type specifier:
// COMDLG_FILTERSPEC rgSpec[] =
//{
//    { szBMP, L"*.raw" },
//    { szAll, L"*.*" },
//};
// 
//  return
//      TRUE - 'Open' selected, pszFilename is valid
//      FALSE-  user cancelled file open
// 
//  This function also asks for verification for overwriting the selected
//  file if it exists (only if bSelectFolder is FALSE)
// 
//****************************************************************
BOOL CCFileSave(HWND hWnd, LPWSTR pszCurrentFilename, LPWSTR* pszFilename,
                BOOL bSelectFolder, int NumTypes, COMDLG_FILTERSPEC* FileTypes,
                LPCWSTR szDefExt)
{
    BOOL bReturnValue = FALSE;

    // Initialize COM
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    if (SUCCEEDED(hr))
    {
        IFileOpenDialog *pFileSave=NULL;

        // Create the FileOpenDialog object.
        hr = CoCreateInstance(CLSID_FileSaveDialog, NULL, CLSCTX_ALL,
            IID_IFileSaveDialog, reinterpret_cast<void**>(&pFileSave));

        if (SUCCEEDED(hr))
        {
            if (wcscmp(pszCurrentFilename, L"") != 0) {
                //parse for just filename
                int err;
                WCHAR Drive[_MAX_DRIVE];
                WCHAR Dir[_MAX_DIR];
                WCHAR Fname[_MAX_FNAME];
                WCHAR Ext[_MAX_EXT];
                WCHAR Directory[MAX_PATH];

                // split apart original filename
                err = _wsplitpath_s(pszCurrentFilename, Drive, _MAX_DRIVE, Dir, _MAX_DIR, Fname,
                    _MAX_FNAME, Ext, _MAX_EXT);
                if (err != 0) {
                    return FALSE;
                }

                err = _wmakepath_s(Directory, _MAX_PATH, Drive, Dir, L"", L"");
                if (err != 0) {
                    return FALSE;
                }
                // if no directory try the global CurrentFilename for directory
                if (wcscmp(Directory, L"") == 0) {
                    err = _wsplitpath_s(szCurrentFilename, Drive, _MAX_DRIVE, Dir, _MAX_DIR, Fname,
                        _MAX_FNAME, Ext, _MAX_EXT);
                    if (err != 0) {
                        return FALSE;
                    }

                    err = _wmakepath_s(Directory, _MAX_PATH, Drive, Dir, L"", L"");
                    if (err != 0) {
                        return FALSE;
                    }
                }

                // if still no directory try the global strAppNameEXE for directory
                if (wcscmp(Directory, L"") == 0) {
                    err = _wsplitpath_s(strAppNameEXE, Drive, _MAX_DRIVE, Dir, _MAX_DIR, Fname,
                        _MAX_FNAME, Ext, _MAX_EXT);
                    if (err != 0) {
                        return FALSE;
                    }

                    err = _wmakepath_s(Directory, _MAX_PATH, Drive, Dir, L"", L"");
                    if (err != 0) {
                        return FALSE;
                    }
                }

                IShellItem* pCurFolder = NULL;
                hr = SHCreateItemFromParsingName(Directory, NULL, IID_PPV_ARGS(&pCurFolder));
                if (SUCCEEDED(hr)) {
                    hr = pFileSave->SetFolder(pCurFolder);
                    hr = pFileSave->SetDefaultFolder(pCurFolder);
                    pCurFolder->Release();
                }
            }

            if (!bSelectFolder) {
                hr = pFileSave->SetOptions(FOS_FORCEFILESYSTEM | FOS_OVERWRITEPROMPT | FOS_PATHMUSTEXIST);
                if (NumTypes != 0 && FileTypes != NULL) {
                    hr = pFileSave->SetFileTypes(NumTypes, FileTypes);
                }
                if (szDefExt != NULL) {
                    hr = pFileSave->SetDefaultExtension(szDefExt);
                }

                if (wcscmp(pszCurrentFilename, L"") != 0)
                {
                    //parse for just filename
                    int err;
                    WCHAR Drive[_MAX_DRIVE];
                    WCHAR Dir[_MAX_DIR];
                    WCHAR Fname[_MAX_FNAME];
                    WCHAR Ext[_MAX_EXT];

                    // split apart original filename
                    err = _wsplitpath_s(pszCurrentFilename, Drive, _MAX_DRIVE, Dir, _MAX_DIR, Fname,
                        _MAX_FNAME, Ext, _MAX_EXT);
                    if (err != 0) {
                        return FALSE;
                    }

                    WCHAR NewFname[_MAX_FNAME];

                    swprintf_s(NewFname, _MAX_FNAME, L"%s%s", Fname, Ext);
                    hr = pFileSave->SetFileName(NewFname);
                }

            } else {
                hr = pFileSave->SetOptions(FOS_PICKFOLDERS | FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST); // This selects only a folder, not a filename
            }
                     
            // Show the Open dialog box.
            hr = pFileSave->Show(NULL);

            // Get the file name from the dialog box.
            if (SUCCEEDED(hr))
            {
                IShellItem* pItem;
                hr = pFileSave->GetResult(&pItem);
                if (SUCCEEDED(hr))
                {
                    // The allocation for pszFIlename by GetDisplayName must be freed when no longer used
                    // using CoTaskMemFree(pszFilename);
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, pszFilename);
                    pItem->Release();
                }
                bReturnValue = TRUE;
            }
            pFileSave->Release();
        }
        // release COM
        CoUninitialize();
    } else {
        MessageBox(hWnd, (LPCWSTR) L"Failed to create save file resource\nFatal Error", (LPCWSTR)L"Save file dialog", IDOK);
        return FALSE;
    }

    return bReturnValue;
}



//*****************************************************************************************
//
// ReadImageHeader
//
// This reads the image header from a file and store it in the passed header structure.
// This also check if the header structure is valid.  If not it will return an error.
// 
// Parameters:
//	WCHAR* Filename				Filename of image file to read header from
//	IMAGINGHEADER* ImageHeader	point to IMAGINGHEADER structure. See imaging.h
//								for definition of structure
// 
//  return value:
//  1 - Success
//  !=1 Error see standardized app error list at top of this source file
//
//*****************************************************************************************
int ReadImageHeader(WCHAR* Filename, IMAGINGHEADER* ImageHeader)
{
    FILE* In;
    errno_t ErrNum;
    size_t iRead;

    ErrNum = _wfopen_s(&In, Filename, L"rb");
    if (In == NULL) {
        return -1;
    }

    iRead = fread(ImageHeader, sizeof(IMAGINGHEADER), 1, In);
    if (iRead != 1) {
        return -2;
    }
    fclose(In);

    if (ImageHeader->Endian != 0 && ImageHeader->Endian != -1) {
        return 0;
    }

    if (ImageHeader->ID != (short)0xaaaa) {
        return 0;
    }

    if (ImageHeader->HeaderSize != sizeof(IMAGINGHEADER)) {
        return 0;
    }

    if (ImageHeader->PixelSize != 1 && ImageHeader->PixelSize != 2 && ImageHeader->PixelSize != 4) {
        return 0;
    }

    return 1;
}

//*****************************************************************************************
//
//	LoadImageFile
// 
//	Load Image file into memory including all frames
//	The Image memory is allocated in this routine.  It must be deleted by the calling processes
//	using 'delete [] ImagePtr' after usage if completed.
//	Note: regardless of Input image PixelSize the Image memory is of type (int)
// 
// Parameters:
//	int** ImagePtr			pointer to (int) array containing input image
//	WCHAR* ImagingFilename	Image file to load
//	IMAGINGHEADER* Header	pointer to IMAGINGHEADER structure of the loaded
//							image file
// 
// return:
//	This function also checks for a valid image header from the file
// 
//	1 - success			'delete [] ImagePtr' must be used to free memory
//	error #				no memory allocated, Header contents invalid
//						see standarized app error number listed above
//
// Usage exmaple:
// 
//		#include "imaging.h"
//		int* Image1;
//		int iRes;
//		IMAGINGHEADER InputHeader;
//		iRes = LoadImageFile(&Image1, ImageInputFile, &InputHeader);
//		if (iRes != 1) {
//			MessageBox(hDlg, L"Input file read error", L"File I/O error", MB_OK);
//			return iRes;
//		}
//		int Pixel;
//		Pixel = Image1[0];
//		delete [] Image1;
//
//*****************************************************************************************
int LoadImageFile(int** ImagePtr, WCHAR* ImagingFilename, IMAGINGHEADER* Header)
{
    FILE* In;
    size_t iRead;

    _wfopen_s(&In, ImagingFilename, L"rb");
    if (In == NULL) {
        *ImagePtr = NULL;
        return -2;
    }

    iRead = fread(Header, sizeof(IMAGINGHEADER), 1, In);
    if (iRead != 1) {
        *ImagePtr = NULL;
        fclose(In);
        return -3;
    }


    if (Header->Endian != 0 && Header->Endian != -1 && Header->ID != 0xaaaa) {
        *ImagePtr = NULL;
        fclose(In);
        return 0;
    }

    if (Header->Xsize <= 0 || Header->Ysize <= 0 || Header->NumFrames <= 0) {
        *ImagePtr = NULL;
        fclose(In);
        return 0;
    }

    if (Header->PixelSize != 1 && Header->PixelSize != 2 && Header->PixelSize != 4) {
        *ImagePtr = NULL;
        fclose(In);
        return 0;
    }

    int* Image;
    int xsize;
    int ysize;
    int NumFrames;
    int PixelSize;
    int Endian;
    xsize = (int)Header->Xsize;
    ysize = (int)Header->Ysize;
    NumFrames = (int)Header->NumFrames;
    PixelSize = (int)Header->PixelSize;
    Endian = (int)Header->Endian;

    Image = new int[(size_t)xsize * (size_t)ysize * (size_t)NumFrames];  // alocate array of 'int's to receive image
    if (Image == NULL) {
        *ImagePtr = NULL;
        fclose(In);
        return -1;
    }

    *ImagePtr = Image;

    // we don't read entire file in one fread since we may need
    // to covert pixel(BYTE,SHORT or LONG) and Endian to get correct 'int'
    PIXEL Pixel;
    for (int i = 0; i < xsize * ysize * NumFrames; i++) {
        iRead = fread(&Pixel, PixelSize, 1, In);
        if (iRead != 1) {
            fclose(In);
            delete[] Image;
            *ImagePtr = NULL;
            return -3;
        }

        if (PixelSize == 1) {
            Image[i] = (int)Pixel.Byte[0];
        }
        else if (PixelSize == 2) {
            if (!Endian) {
                int swap;
                swap = Pixel.Byte[0];
                Pixel.Byte[0] = Pixel.Byte[1];
                Pixel.Byte[1] = swap;
            }
            Image[i] = (int)Pixel.uShort;
        }
        else {
            if (!Endian) {
                int swap;
                swap = Pixel.Byte[0];
                Pixel.Byte[0] = Pixel.Byte[3];
                Pixel.Byte[3] = swap;
                swap = Pixel.Byte[1];
                Pixel.Byte[1] = Pixel.Byte[2];
                Pixel.Byte[2] = swap;
            }
            Image[i] = (int)Pixel.Long;
        }
    }
    fclose(In);
    // calling routine is responsible for deleting 'Image' memory

    return 1;
}

//*****************************************************************************************
//
//	SaveImageFile
// 
//	Save Image file memory including all frames
//	Note: regardless of Input image PixelSize the Image memory is of type (int)
// 
// Parameters:
//	int** ImagePtr			pointer to (int) array containing output image
//	WCHAR* OutputFilename	Image file to save
//	IMAGINGHEADER* Header	pointer to IMAGINGHEADER structure of the loaded
//							image file
// 
// return:
//	This function also checks for a valid image header from the file
// 
//	1 - success
//	error #				no memory allocated, Header contents invalid
//						see standarized app error number listed above
//
//*****************************************************************************************
int SaveImageFile(HWND hDlg, int* OutputImage, WCHAR* OutputFilename, IMAGINGHEADER* Header)
{
    errno_t ErrNum;
    PIXEL Pixel;
    FILE* Out;

    ErrNum = _wfopen_s(&Out, OutputFilename, L"wb");
    if (Out == NULL) {
        MessageBox(hDlg, L"Could not open output file", L"File I/O", MB_OK);
        return APPERR_FILEOPEN;
    }

    //write output image
    fwrite(Header, sizeof(IMAGINGHEADER), 1, Out);

    // write image
    for (int i = 0; i < (Header->Xsize * Header->Ysize * Header->NumFrames); i++) {
        Pixel.Long = (long)OutputImage[i];
        if (Header->PixelSize == 1) {
            if (Pixel.Long > 255) Pixel.Long = 255;
            fwrite(&Pixel.Byte, 1, 1, Out);
        }
        else if (Header->PixelSize == 2) {
            if (Pixel.Long > 65535) Pixel.Long = 65535;
            fwrite(&Pixel.uShort, 2, 1, Out);
        }
        else {
            fwrite(&Pixel.Long, 4, 1, Out);
        }
    }
    fclose(Out);

//    if (DisplayResults) {
//        DisplayImage(OutputFilename);
//    }

    return APP_SUCCESS;
}

//****************************************************************
//
//  SaveBMP
// 
//  This a generic function is used to export a 'image file' to a BMP file type.
//  It does not have any uer interaction.
// 
//  Parmeters:
//      Filename - BMP output file
//      InputFile - Image file for to be exported
//      RGBframes - RBG/Greyscale interpetation flag (see below)
//                  This flag is ignored if the # of frames in the
//                  image is not a multiple of 3.
//      AutoScale - auto scale the output image
// 
//  Xsize of image must be <= 8192
// 
//  Input image pixel supported 8,16,32 bit
// 
//  2 types of output BMP files are supported:
//
//  Input parameter RBGframes FALSE 
//  8 bit per pixel, first frame from file
//      AutoScale parameter TRUE, auto scale as greyscale image, 0 to 255
//      AutoScale parameter only applies to 8 bit input image files.
//      16 and 24 bit images are always automatically scaled as greyscale
//      image, 0 to 255.  This is because there is no BMP file format for 
//      16 or 32 bit monochrome images
//
//  Input parameter RBGframes TRUE
//  24 bit per pixel, RGB interpetation, requires a 3 frame image file
//          a frame is used for each color, 1st frame Red, 2nd frame Green
//          3rd frame Blue, autoscaling stretchs each color independently
//          from the others. Only the first 3 frames are used.
//
//  If input image is odd columns in size it is 0 padded to even size.
//  
//  return value:
//  1 - Success
//  see standardized app error list at top of this source file
//
//****************************************************************
int SaveBMP(WCHAR* Filename, WCHAR* InputFile,int RGBframes, int AutoScale)
{
    int iRes;
    int* InputImage;
    IMAGINGHEADER ImageHeader;
    BITMAPFILEHEADER BMPheader;
    BITMAPINFOHEADER BMPinfoheader;
    RGBQUAD* ColorTable = NULL;
    BYTE* BMPimage;

    iRes = LoadImageFile(&InputImage, InputFile, &ImageHeader);
    if (iRes != 1) {
        return iRes;
    }

    if (ImageHeader.PixelSize > 2) {
        return 0;
    }

    if (ImageHeader.Xsize > 8192) {
        return 0;
    }

    if (RGBframes && ImageHeader.NumFrames%3!=0) {
        RGBframes = 0;
    }

    DWORD BMPimageBytes;
    int biWidth;

    if (RGBframes) {
        //
        //  24 bpp BMP output type
        // 
        // Frame 1 RED
        // Frame 2 Green
        // Frame 3 BLUE
        //
        // convert image data to 24 bit DIB/BMP format.
        // If xsize is odd add pad to even size.
        // Pad as required in BMP 'stride' size.
        //  
        // only three frames file saved.
        //
        // scale display using RGBQUAD color map
        //
        int RedMin, RedMax;
        int GreenMin, GreenMax;
        int BlueMin, BlueMax;
        int RedOffset;
        int GreenOffset;
        int BlueOffset;
        int ImagePixelRed;
        int ImagePixelGreen;
        int ImagePixelBlue;
        int InputFrameSize;
        int Stride;
        float ScaleRed, OffsetRed;
        float ScaleGreen, OffsetGreen;
        float ScaleBlue, OffsetBlue;

        InputFrameSize = ImageHeader.Xsize * ImageHeader.Ysize;

        if (AutoScale) {
            // scan image for red,green,blue stats for scaling
            RedOffset=0;
            GreenOffset = InputFrameSize;
            BlueOffset = 2 * InputFrameSize;

            if (InputImage[RedOffset] < 0) {
                RedMin = RedMax = 0;
            }
            else if (InputImage[RedOffset] > 255) {
                RedMin = RedMax = 255;
            }
            else {
                RedMin = RedMax = InputImage[RedOffset];
            }

            if (InputImage[GreenOffset] < 0) {
                GreenMin = GreenMax = 0;
            }
            else if (InputImage[GreenOffset] > 255) {
                GreenMin = GreenMax = 255;
            }
            else {
                GreenMin = GreenMax = InputImage[GreenOffset];
            }

            if (InputImage[BlueOffset] < 0) {
                BlueMin = BlueMax = 0;
            }
            else if (InputImage[BlueOffset] > 255) {
                BlueMin = BlueMax = 255;
            }
            else {
                BlueMin = BlueMax = InputImage[BlueOffset];
            }

            for (int i = 0; i < InputFrameSize; i++) {
                ImagePixelRed = InputImage[i + RedOffset];
                ImagePixelGreen = InputImage[i + GreenOffset];
                ImagePixelBlue = InputImage[i + BlueOffset];

                if (ImagePixelRed < RedMin) 
                    RedMin = ImagePixelRed;
                if (ImagePixelGreen < GreenMin) 
                    GreenMin = ImagePixelGreen;
                if (ImagePixelBlue < BlueMin) 
                    BlueMin = ImagePixelBlue;

                if (ImagePixelRed > RedMax) 
                    RedMax = ImagePixelRed;
                if (ImagePixelGreen > GreenMax) 
                    GreenMax = ImagePixelGreen;
                if (ImagePixelBlue > BlueMax) 
                    BlueMax = ImagePixelBlue;
            }
            // calculate scaling
            ScaleRed = (float)255.0 / ((float)RedMax - (float)RedMin);
            OffsetRed = (float)0.0 - (ScaleRed * (float)RedMin);

            ScaleGreen = (float)255.0 / ((float)GreenMax - (float)GreenMin);
            OffsetGreen = (float)0.0 - (ScaleRed * (float)GreenMin);

            ScaleBlue = (float)255.0 / ((float)BlueMax - (float)BlueMin);
            OffsetBlue = (float)0.0 - (ScaleBlue * (float)BlueMin);
        }
        else {
            // No Scaling
            ScaleRed = 1.0;
            ScaleBlue = 1.0;
            ScaleGreen = 1.0;
            OffsetRed = 0.0;
            OffsetBlue = 0.0;
            OffsetGreen = 0.0;
        }

        // correct for odd column size
        biWidth = ImageHeader.Xsize;
        if (biWidth % 2 != 0) {
            // make sure bitmap width is even
            biWidth++;
        }

        // BMP files have a specific requirement for # of bytes per line
        // This is called stride.  The formula used is from the specification.
        Stride = ((((biWidth * 24) + 31) & ~31) >> 3); // 24 bpp
        BMPimageBytes = Stride * ImageHeader.Ysize; // size of image in bytes

        // allocate zero paddded image array
        BMPimage = (BYTE*)calloc(BMPimageBytes, 1);
        if (BMPimage == NULL) {
            delete[] InputImage;
            return -1;
        }

        int BMPOffset;
        BYTE PixelRed, PixelGreen, PixelBlue;

        // copy input image to BMPimage DIB format
        for (int y = 0; y < ImageHeader.Ysize; y++) {
            BlueOffset = y * ImageHeader.Xsize;
            GreenOffset = y * ImageHeader.Xsize + InputFrameSize;
            RedOffset = y * ImageHeader.Xsize + (2* InputFrameSize);
            BMPOffset = y * Stride;
            for (int x = 0; x < ImageHeader.Xsize; x++) {
                ImagePixelRed = InputImage[RedOffset + x];
                ImagePixelGreen = InputImage[GreenOffset + x];
                ImagePixelBlue = InputImage[BlueOffset + x];
                // apply scaling
                ImagePixelRed = (int)(ScaleRed * (float)ImagePixelRed + OffsetRed + 0.5);
                if (ImagePixelRed < 0) {
                    PixelRed = 0;
                }
                else if (ImagePixelRed > 255) {
                    PixelRed = 255;
                }
                else {
                    PixelRed = ImagePixelRed;
                }

                ImagePixelGreen = (int)(ScaleGreen * (float)ImagePixelGreen + OffsetGreen + 0.5);
                if (ImagePixelGreen < 0) {
                    PixelGreen = 0;
                }
                else if (ImagePixelGreen > 255) {
                    PixelGreen = 255;
                }
                else {
                    PixelGreen = ImagePixelGreen;
                }

                ImagePixelBlue = (int)(ScaleBlue * (float)ImagePixelBlue + OffsetBlue + 0.5);
                if (ImagePixelBlue < 0) {
                    PixelBlue = 0;
                }
                else if (ImagePixelBlue > 255) {
                    PixelBlue = 255;
                }
                else {
                    PixelBlue = ImagePixelBlue;
                }
                BMPimage[BMPOffset + (x * 3)] = PixelRed;
                BMPimage[BMPOffset + (x * 3) + 1] = PixelGreen;
                BMPimage[BMPOffset + (x * 3) + 2] = PixelBlue;
            }
        }
    }
    else {
        //
        // convert image data to 8 bit DIB/BMP format
        // 
        // If xsize is odd add pad to even size.
        // Pad as required in BMP 'stride' size.
        //  
        // only first frame file saved.
        //
        // 8 bit input image, scale display using RGBQUAD color map, do not scale image data
        // 16 or 32 bit, RGBQUAD color map is scaled 0 to 255 greyscale, input image data scaled to 8 bits
        //
        int PixelMin, PixelMax;
        int InputFrameSize;
        int Stride;
        float Scale, Offset;
        int MaxPixel = 255;
        
        if (ImageHeader.PixelSize > 1) {
            AutoScale = 1;
        }

        if (InputImage[0] < 0) {
            PixelMin = PixelMax = 0;
        }
        PixelMin = PixelMax = InputImage[0];

        InputFrameSize = ImageHeader.Xsize * ImageHeader.Ysize;
        // scan image for scaling
        for (int i=0; i < InputFrameSize; i++) {
            if (InputImage[i] < 0) {
                InputImage[i] = 0;
            }
            if (InputImage[i] < PixelMin) {
                PixelMin = InputImage[i];
            }
            if (InputImage[i] > PixelMax) {
                PixelMax = InputImage[i];
            }
        }

        if (AutoScale) {
            // compute scaling: Scale, Offset
            // this is only used in the RGBQUAD color map for the image
            // It does not change the pixel value
            if (PixelMax == PixelMin) {
                // array is all the same value
                // Make Image all white
                Scale = 0;
                Offset = 255;
            }
            else {
                Scale = (float)255.0 / ((float)PixelMax - (float)PixelMin);
                Offset = (float)0.0 - (Scale * (float)PixelMin);
            }
        }
        else {
            // this can only happens if input image is 8 bit pixels
            Offset = 0.0;
            Scale = 1.0;
        }

        // correct for odd column size
        biWidth = ImageHeader.Xsize;
        if (biWidth % 2 != 0) {
            // make sure bitmap width is even
            biWidth++;
        }

        // BMP files have a specific requirement for # of bytes per line
        // This is called stride.  The formula used is from the specification.
        Stride = ((((biWidth * 8) + 31) & ~31) >> 3); // 8 bpp
        BMPimageBytes = Stride * ImageHeader.Ysize; // size of image in pixels, 8bpp

        // allocate zero paddded image array
        BMPimage = (BYTE*) calloc(BMPimageBytes, 1);
        if (BMPimage == NULL) {
            delete[] InputImage;
            return -1;
        }

        int InputOffset;
        int BMPOffset;
        int ImagePixel;

        // copy image to BMPimage
        for (int y = 0; y < ImageHeader.Ysize; y++) {
            InputOffset = y * ImageHeader.Xsize;
            BMPOffset = y * Stride;
            for (int x = 0; x < ImageHeader.Xsize; x++) {
                ImagePixel = InputImage[InputOffset + x];
                if (ImageHeader.PixelSize > 1) {
                    ImagePixel = (int)(Scale * (float)ImagePixel + Offset + 0.5);
                }
                // copy results into BMPimage
                BMPimage[BMPOffset + x] = (BYTE)ImagePixel;
            }
        }

        // generate RGBDQUAD colormaps
        int k;
        ColorTable = new RGBQUAD[256];
        if (ImageHeader.PixelSize == 1) {
            for (int i = 0; i <= 255; i++) {
                k = (int)(Scale * (float)i + Offset + 0.5);
                if (k < 0) k = 0;
                if (k > 255) k = 255;
                ColorTable[i].rgbBlue = k;
                ColorTable[i].rgbGreen = k;
                ColorTable[i].rgbRed = k;
                ColorTable[i].rgbReserved = 0;
            }
        }
        else {
            for (int i = 0; i <= 255; i++) {
                ColorTable[i].rgbBlue = i;
                ColorTable[i].rgbGreen = i;
                ColorTable[i].rgbRed = i;
                ColorTable[i].rgbReserved = 0;
            }

        }
    }
    delete[] InputImage;

    // fill in BMPheader

    BMPheader.bfType = 0x4d42;  // required ID
    if (!RGBframes) {
        // 8 bpp colormap
        BMPheader.bfSize = (DWORD)(sizeof(BMPheader) + sizeof(BMPinfoheader) + sizeof(RGBQUAD) * 256) + BMPimageBytes;
    }
    else {
        // 24 bit bpp does not have a colormap
        BMPheader.bfSize = (DWORD)(sizeof(BMPheader) + sizeof(BMPinfoheader)) + BMPimageBytes;
    }
    BMPheader.bfReserved1 = 0;
    BMPheader.bfReserved2 = 0;
    if (!RGBframes) {
        // 8 bpp colormap
        BMPheader.bfOffBits = (DWORD)(sizeof(BMPheader) + sizeof(BMPinfoheader) + sizeof(RGBQUAD) * 256);
    }
    else {
        // 24 bit bpp does not have a colormap
        BMPheader.bfOffBits = (DWORD)(sizeof(BMPheader) + sizeof(BMPinfoheader));
    }

    // fill in BMPinfoheader
    BMPinfoheader.biSize = (DWORD) sizeof(BMPinfoheader);
    BMPinfoheader.biWidth = (LONG) biWidth; // calculated and then padded in needed
    BMPinfoheader.biHeight = (LONG)-ImageHeader.Ysize;
    BMPinfoheader.biPlanes = 1;
    if (RGBframes) {
        BMPinfoheader.biBitCount = 24;
    }
    else {
        BMPinfoheader.biBitCount = 8;
    }
    BMPinfoheader.biCompression = BI_RGB;
    BMPinfoheader.biSizeImage = BMPimageBytes;
    BMPinfoheader.biXPelsPerMeter = 2834;
    BMPinfoheader.biYPelsPerMeter = 2834;
    BMPinfoheader.biClrUsed = 0;
    BMPinfoheader.biClrImportant = 0;

    // write BMP file

    FILE* Out;
    errno_t ErrNum;
    ErrNum = _wfopen_s(&Out, Filename, L"wb");
    if (Out == NULL) {
        free(BMPimage);
        return -2;
    }

    // write the BMPheader
    fwrite(&BMPheader, sizeof(BMPheader), 1, Out);

    // write the BMPinfoheader
    fwrite(&BMPinfoheader, sizeof(BMPinfoheader), 1, Out);

    // write color map only if greyscale image
    if (!RGBframes) {
        fwrite(ColorTable, sizeof(RGBQUAD), 256, Out);
        delete[] ColorTable;
    }

    // write the image data
    int Address = 0;
    for (int i = 0; i < (int)BMPimageBytes; i++) {
        fwrite(&BMPimage[i], 1, 1, Out);
        Address++;
    }

    free(BMPimage);
    fclose(Out);

    if (AutoPNG) {
        if (SaveBMP2PNG(Filename) != 1) {
            return 0;
        }
    }

    return 1;
}

//****************************************************************
//
//  SaveTXT
// 
//  This a generic function is used to export a 'image file' to a text file.
//  It does not have any uer interaction.
// 
//  Parmeters:
//      Filename - text output file
//      InputFile - Image file for to be exported
//
// The output file is in a text format with whitespace as delimiters
// Each row is image is output as a single line in the text file.
// Each pixel in a row is output as a decmial number. 
// The number range of a pixel is determined by the pixel size of the
// input image:
//      1 byte - 0 to 255
//      2 byte - 0 to 65535
//      4 byte - 0 to 2147483648
// If there are multiple frames there is a blank line between frames. 
//  
//  return value:
//  1 - Success
//  see standardized app error list at top of this source file
//
//****************************************************************
int SaveTXT(WCHAR* Filename, WCHAR* InputFile)
{
    int iRes;
    int* InputImage;
IMAGINGHEADER ImageHeader;

iRes = LoadImageFile(&InputImage, InputFile, &ImageHeader);
if (iRes != 1) {
    return iRes;
}

FILE* Out;
errno_t ErrNum;
int Address;

ErrNum = _wfopen_s(&Out, Filename, L"w");
if (Out == NULL) {
    delete[] InputImage;
    return -2;
}

// save file in text format, blank line between frames
Address = 0;
int Pixel;

for (int Frame = 0; Frame < ImageHeader.NumFrames; Frame++) {
    for (int y = 0; y < ImageHeader.Ysize; y++) {
        for (int x = 0; x < ImageHeader.Xsize; x++) {
            Pixel = InputImage[Address];
            // make sure pixel is not less than 0
            if (Pixel < 0) Pixel = 0;
            if (ImageHeader.PixelSize == 1) {
                // clip value to match pixel size
                if (Pixel > 255) Pixel = 255;
                fprintf(Out, "%3d ", Pixel);
            }
            else if (ImageHeader.PixelSize == 1) {
                // clip value to match pixel size
                if (Pixel > 65535) Pixel = 65535;
                fprintf(Out, "%5d ", Pixel);
            }
            else {
                fprintf(Out, "%7d ", Pixel);
            }
            Address++;
        }
        fprintf(Out, "\n");
    }
    fprintf(Out, "\n");
}
fclose(Out);
delete[] InputImage;

return 1;
}

//****************************************************************
//
//  Hex2Binary
// 
//****************************************************************
int HEX2Binary(HWND hWnd)
{
    WCHAR InputFilename[MAX_PATH];
    WCHAR OutputFilename[MAX_PATH];

    GetPrivateProfileString(L"Hex2Binary", L"InputFile", L"*.txt", InputFilename, MAX_PATH, (LPCTSTR)strAppNameINI);
    GetPrivateProfileString(L"Hex2Binary", L"OutputFile", L"", OutputFilename, MAX_PATH, (LPCTSTR)strAppNameINI);

    PWSTR pszFilename;
    COMDLG_FILTERSPEC txtType[] =
    {
         { L"Text files", L"*.txt" },
         { L"All Files", L"*.*" }
    };

    COMDLG_FILTERSPEC AllType[] =
    {
         { L"BMP files", L"*.bmp" },
         { L"Image files", L"*.raw" },
         { L"All Files", L"*.*" }
    };

    if (!CCFileOpen(hWnd, InputFilename, &pszFilename, FALSE, 2, txtType, L".bmp")) {
        return 1;
    }
    wcscpy_s(InputFilename, pszFilename);
    CoTaskMemFree(pszFilename);

    if (!CCFileSave(hWnd, OutputFilename, &pszFilename, FALSE, 3, AllType, L"")) {
        return 1;
    }
    wcscpy_s(OutputFilename, pszFilename);
    CoTaskMemFree(pszFilename);

    WritePrivateProfileString(L"Hex2Binary", L"InputFile", InputFilename, (LPCTSTR)strAppNameINI);
    WritePrivateProfileString(L"Hex2Binary", L"OutputFile", OutputFilename, (LPCTSTR)strAppNameINI);

    FILE* Input;
    FILE* Output;
    errno_t ErrNum;
    BYTE HexValue;
    int HexRead;
    int iRes;

    ErrNum = _wfopen_s(&Input, InputFilename, L"rb");
    if (Input ==NULL) {
        return -2;
    }

    ErrNum = _wfopen_s(&Output, OutputFilename, L"wb");
    if (Output == NULL) {
        return -2;
    }

    while (!feof(Input)) {
        iRes = fscanf_s(Input, "%2x", &HexRead);
        if (iRes != 1) {
            break;
        }
        HexValue = (BYTE)HexRead;
        fwrite(&HexValue, 1, 1, Output);
    }

    fclose(Output);
    fclose(Input);

    return 1;
}

//****************************************************************
//
//  CamIRaImport
// 
//****************************************************************
int CamIRaImport(HWND hWnd)
{
    // This structure must be exact since it is saved in a file.
#pragma pack(1)
    struct IMAGE_HEADER {
        short FirstWord;	// -1
        short Xsize;		// number of pixels in a line (# of columns)
        short Ysize;		// number of lines (# of rows)
        short PixelSize;	// # of bytes/pixel
        short Dummy1[16];   // ignore these
        short SelectFrames;	// <-10 use NumFrames1,  >10 use NumFrames2
        short NumFrames1;	// # of image frames in file. if SelectFrames <= 10 use this
        short Dummy2[206];  // ignore these
        INT32 NumFrames2;	// # of frames in file. If SelectFrames > 10 use this
        short Dummy3[26];   // ignore these
    } CamIRaHeader;
#pragma pack()

    WCHAR InputFilename[MAX_PATH];
    WCHAR OutputFilename[MAX_PATH];

    GetPrivateProfileString(L"CamIRaImport", L"InputFile", L"*.img", InputFilename, MAX_PATH, (LPCTSTR)strAppNameINI);
    GetPrivateProfileString(L"CamIRaImport", L"OutputFile", L"", OutputFilename, MAX_PATH, (LPCTSTR)strAppNameINI);

    PWSTR pszFilename;
    COMDLG_FILTERSPEC txtType[] =
    {
         { L"Text files", L"*.img" },
         { L"All Files", L"*.*" }
    };

    COMDLG_FILTERSPEC AllType[] =
    {
         { L"Image files", L"*.raw" },
         { L"All Files", L"*.*" }
    };

    if (!CCFileOpen(hWnd, InputFilename, &pszFilename, FALSE, 2, txtType, L".img")) {
        return 1;
    }
    wcscpy_s(InputFilename, pszFilename);
    CoTaskMemFree(pszFilename);

    if (!CCFileSave(hWnd, OutputFilename, &pszFilename, FALSE, 2, AllType, L".raw")) {
        return 1;
    }
    wcscpy_s(OutputFilename, pszFilename);
    CoTaskMemFree(pszFilename);

    WritePrivateProfileString(L"CamIRaImport", L"InputFile", InputFilename, (LPCTSTR)strAppNameINI);
    WritePrivateProfileString(L"CamIRaImport", L"OutputFile", OutputFilename, (LPCTSTR)strAppNameINI);

    FILE* Input;
    FILE* Output;
    errno_t ErrNum;

    ErrNum = _wfopen_s(&Input, InputFilename, L"rb");
    if (Input == NULL) {
        return -2;
    }

    int iRes;
    int HeaderLen;
    HeaderLen = sizeof(CamIRaHeader);
    iRes = (int)fread(&CamIRaHeader, HeaderLen, 1, Input);
    if (iRes != 1) {
        fclose(Input);
        return -4;
    }

    if (CamIRaHeader.FirstWord != -1 && CamIRaHeader.PixelSize!=1 && CamIRaHeader.PixelSize!=2) {
        fclose(Input);
        return -4;
    }
    
    int NumFrames;
    if (CamIRaHeader.SelectFrames <= 10 && CamIRaHeader.SelectFrames!=0) {
        NumFrames = CamIRaHeader.NumFrames1;
    }
    else {
        NumFrames = CamIRaHeader.NumFrames2;
    }
    if (NumFrames > 32767 || CamIRaHeader.PixelSize > 2) {
        fclose(Input);
        return -4;
    }

    // create ImageHeader
    IMAGINGHEADER ImgHeader;

    ImgHeader.Endian = (short)-1;  // PC format
    ImgHeader.HeaderSize = (short)sizeof(IMAGINGHEADER);
    ImgHeader.ID = (short)0xaaaa;
    ImgHeader.Version = (short)1;
    ImgHeader.NumFrames = (short)NumFrames;
    ImgHeader.PixelSize = CamIRaHeader.PixelSize;
    ImgHeader.Xsize = CamIRaHeader.Xsize;
    ImgHeader.Ysize = CamIRaHeader.Ysize;
    ImgHeader.Padding[0] = 0;
    ImgHeader.Padding[1] = 0;
    ImgHeader.Padding[2] = 0;
    ImgHeader.Padding[3] = 0;
    ImgHeader.Padding[4] = 0;
    ImgHeader.Padding[5] = 0;

    // open output file
    ErrNum = _wfopen_s(&Output, OutputFilename, L"wb");
    if (Output == NULL) {
        fclose(Input);
        return -2;
    }

    // write ImageHeader
    fwrite(&ImgHeader, sizeof(ImgHeader), 1, Output);

    // copy Input file to output file
    for (int Frame = 0; Frame < NumFrames; Frame++) {
        for (int y = 0; y < ImgHeader.Ysize; y++) {
            for (int x = 0; x < ImgHeader.Xsize; x++) {
                if (ImgHeader.PixelSize == 1) {
                    BYTE Pixel;
                    iRes = (int)fread(&Pixel, 1, 1, Input);
                    if (iRes != 1) {
                        fclose(Input);
                        fclose(Output);
                        return -4;
                    }
                    fwrite(&Pixel, 1, 1, Output);
                }
                else {
                    short Pixel;
                    iRes = (int)fread(&Pixel, 2, 1, Input);
                    if (iRes != 1) {
                        fclose(Input);
                        fclose(Output);
                        return -4;
                    }
                    fwrite(&Pixel, 2, 1, Output);
                }
            }
        }
    }
    wcscpy_s(szCurrentFilename, OutputFilename);
    fclose(Output);
    fclose(Input);
    return 1;
}

//****************************************************************
//
//  GetFileSize
// 
//****************************************************************
int GetFileSize(WCHAR* szString)
{
    int FileSize = 0;
    int iRes;
    BYTE Junk;
    FILE* In;
    errno_t ErrNum;

    ErrNum = _wfopen_s(&In, szString, L"rb");
    if (In == NULL) {
        return -2;
    }

    while (!feof(In)) {
        iRes = (int)fread(&Junk, 1, 1, In);
        if (iRes != 1) {
            break;
        }
        FileSize++;
    }

    fclose(In);
    return FileSize;
}

//****************************************************************
//
//  SaveBMP2PNG
// 
// This is based on a solution provided at:
// https://learn.microsoft.com/en-us/windows/win32/gdiplus/-gdiplus-converting-a-bmp-image-to-a-png-image-use
// 
//****************************************************************
// using namespace Gdiplus;

int SaveBMP2PNG(WCHAR* Filename)
{
    // Initialize GDI+.
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    CLSID   encoderClsid;
    Gdiplus::Status  stat;

    // load BMP file to image
    Gdiplus::Image* image = new Gdiplus::Image(Filename);

    // Get the CLSID of the PNG encoder.
    GetEncoderClsid(L"image/png", &encoderClsid);
    // generate .png name version of Filename
    {
        //parse for just filename
        int err;
        WCHAR Drive[_MAX_DRIVE];
        WCHAR Dir[_MAX_DIR];
        WCHAR Fname[_MAX_FNAME];
        WCHAR Ext[_MAX_EXT];
        WCHAR PNGfilename[MAX_PATH];

        // split apart original filename
        err = _wsplitpath_s(Filename, Drive, _MAX_DRIVE, Dir, _MAX_DIR, Fname,
            _MAX_FNAME, Ext, _MAX_EXT);
        if (err != 0) {
            return 0;
        }

        err = _wmakepath_s(PNGfilename, _MAX_PATH, Drive, Dir, Fname, L".png");
        if (err != 0) {
            return 0;
        }

        stat = image->Save(PNGfilename, &encoderClsid, NULL);
    }
    delete image;
    Gdiplus::GdiplusShutdown(gdiplusToken);

    if (stat == Gdiplus::Ok) {
        return 1;
    }

    return 0;
}

//****************************************************************
//
//  SaveImageBMP
// 
//****************************************************************
int SaveImageBMP(WCHAR* Filename,COLORREF* Image, int ImageXextent, int ImageYextent) {
    if (wcslen(Filename) == 0) {
        return APPERR_PARAMETER;
    }

    int biWidth;
    int Stride;
    int BMPimageBytes;
    BYTE* BMPimage = NULL;

    // correct for odd column size

    biWidth = ImageXextent;
    if (biWidth % 2 != 0) {
        // make sure bitmap width is even
        biWidth++;
    }

    // BMP files have a specific requirement for # of bytes per line
    // This is called stride.  The formula used is from the specification. 
    Stride = ((((biWidth * 24) + 31) & ~31) >> 3); // 24 bpp
    BMPimageBytes = Stride * ImageYextent; // size of image in bytes

    // allocate zero paddded image array
    BMPimage = (BYTE*)calloc(BMPimageBytes, 1);
    if (BMPimage == NULL) {
        return APPERR_MEMALLOC;
    }

    int BMPOffset;
    int Offset;
    union {
        COLORREF Color;
        RGBQUAD rgb;
    } iColor;

    // copy input COLORREF image to BMPimage DIB format
    for (int y = 0; y < ImageYextent; y++) {
        Offset = y * ImageXextent;
        BMPOffset = y * Stride;
        for (int x = 0; x < ImageXextent; x++) {
            iColor.Color = Image[Offset + x];
            BMPimage[BMPOffset + (x * 3)] = iColor.rgb.rgbRed;
            BMPimage[BMPOffset + (x * 3) + 1] = iColor.rgb.rgbGreen;
            BMPimage[BMPOffset + (x * 3) + 2] = iColor.rgb.rgbBlue;
        }
    }

    // fill in BMPheader
    BITMAPFILEHEADER BMPheader;

    BMPheader.bfType = 0x4d42;  // required ID
    BMPheader.bfSize = (DWORD)(sizeof(BMPheader) + sizeof(BITMAPINFOHEADER)) + BMPimageBytes;
    BMPheader.bfReserved1 = 0;
    BMPheader.bfReserved2 = 0;
    BMPheader.bfOffBits = (DWORD)(sizeof(BMPheader) + sizeof(BITMAPINFOHEADER));

    // fill in BMPinfoheader
    BITMAPINFOHEADER BMPinfoheader;

    BMPinfoheader.biSize = (DWORD)sizeof(BMPinfoheader);
    BMPinfoheader.biWidth = (LONG)biWidth; // calculated and then padded if needed
    BMPinfoheader.biHeight = (LONG)-ImageYextent;
    BMPinfoheader.biPlanes = 1;
    BMPinfoheader.biBitCount = 24;
    BMPinfoheader.biCompression = BI_RGB;
    BMPinfoheader.biSizeImage = BMPimageBytes;
    BMPinfoheader.biXPelsPerMeter = 2834;
    BMPinfoheader.biYPelsPerMeter = 2834;
    BMPinfoheader.biClrUsed = 0;
    BMPinfoheader.biClrImportant = 0;

    // write BMP file

    FILE* Out;
    errno_t ErrNum;
    ErrNum = _wfopen_s(&Out, Filename, L"wb");
    if (Out == NULL) {
        free(BMPimage);
        return APPERR_FILEOPEN;
    }

    // write the BMPheader
    fwrite(&BMPheader, sizeof(BMPheader), 1, Out);

    // write the BMPinfoheader
    fwrite(&BMPinfoheader, sizeof(BMPinfoheader), 1, Out);

    // write the image data
    int Address = 0;
    for (int i = 0; i < (int)BMPimageBytes; i++) {
        fwrite(&BMPimage[i], 1, 1, Out);
        Address++;
    }

    free(BMPimage);
    fclose(Out);

    if (AutoPNG) {
        if (SaveBMP2PNG(Filename) != 1) {
            return 0;
        }
    }
    return APP_SUCCESS;
}

//****************************************************************
//
//  LoadBMPfile
// 
//****************************************************************
int  ReadBMPfile(int** ImagePtr, WCHAR* InputFilename, IMAGINGHEADER* ImgHeader)
{
    int Invert = 0;

    // open BMP file
    FILE* BMPfile;
    errno_t ErrNum;
    int iRes;

    ErrNum = _wfopen_s(&BMPfile, InputFilename, L"rb");
    if (!BMPfile) {
        return APPERR_FILEOPEN;
    }

    // read BMP headers
    BITMAPFILEHEADER BMPheader;
    BITMAPINFOHEADER BMPinfoheader;
    int StrideLen;
    int* Image;
    BYTE* Stride;

    iRes = (int)fread(&BMPheader, sizeof(BITMAPFILEHEADER), 1, BMPfile);
    if (iRes != APP_SUCCESS) {
        fclose(BMPfile);
        return APPERR_FILETYPE;
    }

    iRes = (int)fread(&BMPinfoheader, sizeof(BITMAPINFOHEADER), 1, BMPfile);
    if (iRes != APP_SUCCESS) {
        fclose(BMPfile);
        return APPERR_FILETYPE;
    }

    // verify this type of file can be imported
    if (BMPheader.bfType != 0x4d42 || BMPheader.bfReserved1 != 0 || BMPheader.bfReserved2 != 0) {
        // this is not a BMP file
        fclose(BMPfile);
        return APPERR_FILETYPE;
    }
    if (BMPinfoheader.biSize != sizeof(BITMAPINFOHEADER)) {
        // this is not a BMP file
        fclose(BMPfile);
        return APPERR_FILETYPE;
    }

    if (BMPinfoheader.biCompression != BI_RGB) {
        // this is wrong type of BMP file
        fclose(BMPfile);
        return APPERR_PARAMETER;
    }
    if (BMPinfoheader.biBitCount != 1 && BMPinfoheader.biBitCount != 8 &&
        BMPinfoheader.biBitCount != 24 && BMPinfoheader.biPlanes != 1) {
        // this is wrong type of BMP file
        fclose(BMPfile);
        return APPERR_PARAMETER;
    }

    // read in image
    int BMPimageBytes;
    int TopDown = 1;
    if (BMPinfoheader.biHeight < 0) {
        TopDown = 0;
        BMPinfoheader.biHeight = -BMPinfoheader.biHeight;
    }

    // BMP files have a specific requirement for # of bytes per line
    // This is called stride.  The formula used is from the specification.
    StrideLen = ((((BMPinfoheader.biWidth * BMPinfoheader.biBitCount) + 31) & ~31) >> 3); // 24 bpp
    BMPimageBytes = StrideLen * BMPinfoheader.biHeight; // size of image in bytes

    // allocate stride
    Stride = new BYTE[(size_t)StrideLen];
    if (Stride == NULL) {
        fclose(BMPfile);
        return APPERR_MEMALLOC;
    }

    int NumFrames = 1;

    if (BMPinfoheader.biBitCount == 1) {
        // This is bit image, has color table, 2 entries
        // Skip the 2 RGBQUAD entries
        if (fseek(BMPfile, sizeof(RGBQUAD) * 2, SEEK_CUR) != 0) {
            delete[] Stride;
            fclose(BMPfile);
            return APPERR_FILETYPE;
        }
        int BitCount;
        int StrideIndex;
        int Offset;

        // allocate Image
        // alocate array of 'int's to receive image
        Image = new int[(size_t)BMPinfoheader.biWidth * (size_t)BMPinfoheader.biHeight];
        if (Image == NULL) {
            delete[] Stride;
            fclose(BMPfile);
            return APPERR_MEMALLOC;
        }

        // BMPimage of BMPimageBytes
        for (int y = 0; y < BMPinfoheader.biHeight; y++) {
            // read stride
            iRes = (int)fread(Stride, 1, StrideLen, BMPfile);
            if (iRes != StrideLen) {
                delete[] Image;
                delete[] Stride;
                fclose(BMPfile);
                return APPERR_FILETYPE;
            }
            BitCount = 0;
            StrideIndex = 0;
            if (TopDown) {
                Offset = ((BMPinfoheader.biHeight - 1) - y) * BMPinfoheader.biWidth;
            }
            else {
                Offset = y * BMPinfoheader.biWidth;
            }
            for (int x = 0; x < BMPinfoheader.biWidth; x++) {
                // split out bit by bit
                Image[Offset + x] = Stride[StrideIndex] & (0x80 >> BitCount);
                if (Image[Offset + x] != 0) {
                    if (Invert == 0) {
                        Image[Offset + x] = 255;
                    }
                    else {
                        Image[Offset + x] = 0;
                    }
                }
                else {
                    if (Invert == 0) {
                        Image[Offset + x] = 0;
                    }
                    else {
                        Image[Offset + x] = 255;
                    }
                }
                BitCount++;
                if (BitCount == 8) {
                    BitCount = 0;
                    StrideIndex++;
                }
            }
        }
    }
    else if (BMPinfoheader.biBitCount == 8) {
        // this is a byte image, has color table, 256 entries
        // Skip the 256 RGBQUAD entries
        if (fseek(BMPfile, sizeof(RGBQUAD) * 256, SEEK_CUR) != 0) {
            delete[] Stride;
            fclose(BMPfile);
            return APPERR_FILETYPE;
        }

        // allocate Image
        // alocate array of 'int's to receive image
        Image = new int[(size_t)BMPinfoheader.biWidth * (size_t)BMPinfoheader.biHeight];
        if (Image == NULL) {
            delete[] Stride;
            fclose(BMPfile);
            return APPERR_MEMALLOC;
        }

        // BMPimage of BMPimageBytes
        int Offset;

        for (int y = 0; y < BMPinfoheader.biHeight; y++) {
            // read stride
            iRes = (int)fread(Stride, 1, StrideLen, BMPfile);
            if (iRes != StrideLen) {
                delete[] Image;
                delete[] Stride;
                fclose(BMPfile);
                return APPERR_FILETYPE;
            }
            if (TopDown) {
                Offset = ((BMPinfoheader.biHeight - 1) - y) * BMPinfoheader.biWidth;
            }
            else {
                Offset = y * BMPinfoheader.biWidth;
            }

            for (int x = 0; x < BMPinfoheader.biWidth; x++) {
                Image[Offset + x] = Stride[x];
            }
        }
    }
    else {
        // this is a 24 bit, RGB image
        // The color table is biClrUsed long
        NumFrames = 3;
        if (BMPinfoheader.biClrUsed != 0) {
            // skip the color table if present
            if (fseek(BMPfile, sizeof(RGBQUAD) * BMPinfoheader.biClrUsed, SEEK_CUR) != 0) {
                delete[] Stride;
                fclose(BMPfile);
                return APPERR_FILETYPE;
            }
        }

        Image = new int[(size_t)BMPinfoheader.biWidth * (size_t)BMPinfoheader.biHeight * (size_t)3];
        if (Image == NULL) {
            delete[] Stride;
            fclose(BMPfile);
            return APPERR_MEMALLOC;
        }

        // BMPimage of BMPimageBytes
        int Offset;
        int RedFrame = 0;
        int GreenFrame = BMPinfoheader.biHeight * BMPinfoheader.biWidth;
        int BlueFrame = BMPinfoheader.biHeight * BMPinfoheader.biWidth * 2;

        for (int y = 0; y < BMPinfoheader.biHeight; y++) {
            // read stride
            iRes = (int)fread(Stride, 1, StrideLen, BMPfile);
            if (iRes != StrideLen) {
                delete[] Image;
                delete[] Stride;
                fclose(BMPfile);
                return APPERR_FILETYPE;
            }
            if (TopDown) {
                Offset = ((BMPinfoheader.biHeight - 1) - y) * BMPinfoheader.biWidth;
            }
            else {
                Offset = y * BMPinfoheader.biWidth;
            }

            for (int x = 0; x < BMPinfoheader.biWidth; x++) {
                Image[BlueFrame + Offset + x] = Stride[x * 3 + 0];
                Image[GreenFrame + Offset + x] = Stride[x * 3 + 1];
                Image[RedFrame + Offset + x] = Stride[x * 3 + 2];
            }
        }
    }

    delete[] Stride;
    fclose(BMPfile);

    *ImagePtr = Image;

    ImgHeader->Endian = (short)-1;  // PC format
    ImgHeader->HeaderSize = (short)sizeof(IMAGINGHEADER);
    ImgHeader->ID = (short)0xaaaa;
    ImgHeader->Version = (short)1;
    ImgHeader->NumFrames = (short)NumFrames;
    ImgHeader->PixelSize = (short)1;
    ImgHeader->Xsize = BMPinfoheader.biWidth;
    ImgHeader->Ysize = BMPinfoheader.biHeight;
    ImgHeader->Padding[0] = 0;
    ImgHeader->Padding[1] = 0;
    ImgHeader->Padding[2] = 0;
    ImgHeader->Padding[3] = 0;
    ImgHeader->Padding[4] = 0;
    ImgHeader->Padding[5] = 0;

    return APP_SUCCESS;
}

//*******************************************************************
//
// SaveBits2Text
// 
// Save byte stream bits to text file
// 
//
//*******************************************************************
int SaveBYTEs2Text(WCHAR* OutputFile, BYTE* ByteStream,
    int NumBytes, int BitOrder)
{
    FILE* Out;
    int CurrentBit = 0;
    errno_t ErrNum;

    // this is a text file
    ErrNum = _wfopen_s(&Out, OutputFile, L"w");
    if (Out == NULL) {
        return APPERR_FILEOPEN;
    }

    for(int i=0; i<NumBytes; i++) {
        BYTE CurrentByte;
        int BitValue;
        int CurrentByteBit = 0;

        CurrentByte = ByteStream[i];
        // process input file bit by bit
        // while extracting use selected bit order in byte, LSB to MSB or MSB to LSB
        // variable Endian is really referring to bit order in byte not byte order in multiple byte sequence
        while (CurrentByteBit < 8) {
            // process data bit by bit in the order of the bit transmission message
            // input file is byte oriented MSB to LSB representing the bit order that the message was received
            // This does not imply any bit ordering in the message itself.
            if (BitOrder == 0) {
                BitValue = CurrentByte & (0x80 >> CurrentByteBit);
            }
            else {
                BitValue = CurrentByte & (0x01 << CurrentByteBit);
            }
            if (!BitValue) {
                fprintf(Out, " 0");
            }
            else {
                fprintf(Out, " 1");
            }
            CurrentByteBit++;
        }
        fprintf(Out, "\n");
    }

    fclose(Out);

    return APP_SUCCESS;
}

//*******************************************************************
//
// ReadBits2Text
// 
// Save byte stream bits to text file
// 
//*******************************************************************
int ReadBYTEs2Text(WCHAR* InputFile, BYTE* ByteStream,
    int NumBytes, int BitOrder)
{
    FILE* In;
    int CurrentByte = 0;
    errno_t ErrNum;

    // this is a text file
    ErrNum = _wfopen_s(&In, InputFile, L"r");
    if (In == NULL) {
        return APPERR_FILEOPEN;
    }

    for (int i = 0; i < NumBytes; i++) {
        int iRes;
        int BitValue;

        for (int CurrentBit = 0; CurrentBit < 8; CurrentBit++) {
            iRes = fscanf_s(In, "%d", &BitValue);
            if (iRes != 1) {
                fclose(In);
                return APPERR_FILEREAD;
            }
            if (BitValue < 0) {
                fclose(In);
                return APPERR_FILEREAD;
            }
            if (BitValue != 0) {
                if (BitOrder == 0) {
                    CurrentByte = CurrentByte | (0x80 >> CurrentBit);
                }
                else {
                    CurrentByte = CurrentByte | (0x01 << CurrentBit);
                }
            }
        }
        ByteStream[i] = CurrentByte;
        CurrentByte = 0;
    }

    fclose(In);

    return APP_SUCCESS;
}

//*******************************************************************
//
// SaveASISbitstream
// 
// Save byte stream bits to text file
// 
//*******************************************************************
int SaveASISbitstream(WCHAR* Filename, BYTE* Header, BYTE* MessageBody, BYTE* Footer)
{
    FILE* Out;

    errno_t ErrNum;
    size_t Length;

    if (Header == nullptr || MessageBody == nullptr || Footer == nullptr) {
        return APPERR_PARAMETER;
    }

    // this is a binary file
    ErrNum = _wfopen_s(&Out, Filename, L"wb");
    if (Out == NULL) {
        return APPERR_FILEOPEN;
    }

    Length = fwrite(Header, 1, 10, Out);
    if (Length != 10) {
        fclose(Out);
        return APPERR_FILEWRITE;
    }

    Length = fwrite(MessageBody, 1, 8192, Out);
    if (Length != 8192) {
        fclose(Out);
        return APPERR_FILEWRITE;
    }

    Length = fwrite(Footer, 1, 10, Out);
    if (Length != 10) {
        fclose(Out);
        return APPERR_FILEWRITE;
    }

    fclose(Out);

    return APP_SUCCESS;
}

//*******************************************************************
//
// SaveHistogramData
// 
// Save histogram data to .csv file
// 
//*******************************************************************
int SaveHistogramData(WCHAR* Filename, BOOL CreateNew, int Index, int* Histogram, int NumEntries)
{
    FILE* Out;
    errno_t ErrNum;

    // disassemble filename, create new filename with .txt extension
    WCHAR NewFilename[MAX_PATH];
    {
        //parse for just filename
        int err;
        WCHAR Drive[_MAX_DRIVE];
        WCHAR Dir[_MAX_DIR];
        WCHAR Fname[_MAX_FNAME];
        WCHAR Ext[_MAX_EXT];

        // split apart original filename
        err = _wsplitpath_s(Filename, Drive, _MAX_DRIVE, Dir, _MAX_DIR, Fname,
            _MAX_FNAME, Ext, _MAX_EXT);
        if (err != 0) {
            return APPERR_FILEOPEN;
        }

        err = _wmakepath_s(NewFilename, _MAX_PATH, Drive, Dir, Fname, L".csv");
        if (err != 0) {
            return APPERR_FILEOPEN;
        }
    }

    if (CreateNew) {
        ErrNum = _wfopen_s(&Out, NewFilename, L"w");
        if (Out == NULL) {
            return APPERR_FILEOPEN;
        }
    }
    else {
        ErrNum = _wfopen_s(&Out, NewFilename, L"a");
        if (Out == NULL) {
            return APPERR_FILEOPEN;
        }
    }

    // write new line of data to file
    fprintf(Out, "%10d,", Index);
    for (int i = 0; i < (NumEntries-1); i++) {
        fprintf(Out, " %10d,", Histogram[i]);
    }
    fprintf(Out, " %10d\n", Histogram[NumEntries-1]);

    fclose(Out);

    return APP_SUCCESS;
}

//*******************************************************************
//
// SaveSnapShot
// 
// Save snapshot of current iteration
// 
//*******************************************************************
int SaveSnapshot(HWND hDlg, int CurrentIteration, int* TheImage, IMAGINGHEADER* BCAimageHeader)
{
    // save current image using output name + iteration number
    WCHAR OutputFilename[MAX_PATH];
    WCHAR NewFilename[MAX_PATH];

    int err;
    WCHAR Drive[_MAX_DRIVE];
    WCHAR Dir[_MAX_DIR];
    WCHAR Fname[_MAX_FNAME];
    WCHAR Ext[_MAX_EXT];

    GetDlgItemText(hDlg, IDC_IMAGE_OUTPUT, OutputFilename, MAX_PATH);

    // split apart original filename
    err = _wsplitpath_s(OutputFilename, Drive, _MAX_DRIVE, Dir, _MAX_DIR, Fname,
        _MAX_FNAME, Ext, _MAX_EXT);
    if (err != 0) {
        MessageBox(hDlg, L"Could not creat output filename", L"BCA save image", MB_OK);
        return APPERR_FILEOPEN;
    }
    // change the fname portion to add _kernel# 1 based
    // use Kernel+1
    WCHAR NewFname[_MAX_FNAME];

    swprintf_s(NewFname, _MAX_FNAME, L"%s_%08d", Fname, CurrentIteration);

    // reassemble filename
    err = _wmakepath_s(NewFilename, _MAX_PATH, Drive, Dir, NewFname, Ext);
    if (err != 0) {
        MessageBox(hDlg, L"Could not creat output filename", L"BCA save image", MB_OK);
        return APPERR_FILEOPEN;
    }

    int iRes;
    iRes = SaveImageFile(hDlg, TheImage, NewFilename, BCAimageHeader);
    if (iRes == APP_SUCCESS && IsDlgButtonChecked(hDlg, IDC_BMP_FILE) == BST_CHECKED) {
        // reassemble filename
        WCHAR BMPFilename[MAX_PATH];
        err = _wmakepath_s(BMPFilename, _MAX_PATH, Drive, Dir, NewFname, L".bmp");
        if (err == 0) {
            iRes = SaveBMP(BMPFilename, NewFilename, FALSE, TRUE);
            if (iRes == APP_SUCCESS) {
                iRes = GetPrivateProfileInt(L"SettingsGlobalDlg", L"AutoPNG", 1, (LPCTSTR)strAppNameINI);
                if (iRes != 0) {
                    SaveBMP2PNG(BMPFilename);
                }
            }
        }
    }
    return APP_SUCCESS;
}
