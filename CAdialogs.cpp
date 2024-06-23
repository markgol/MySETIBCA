//
// MySETIBCA, an application for decoding, encoding message images using 
// a block cellular automata like what was used in the 'A Sign inSpace' project message
// 
// CAdialogs.cpp
// (C) 2024, Mark Stegall
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
// If not, see < https://www.gnu.org/licenses/>.
// 
// This file contains the dialog callback procedures for the Cellular Automata tools menu
// 
// V1.0.0	2024-06-21	Initial release
// V1.0.1   2024-06-23  Changed BCA layer raw file input of color based raw image(3 frames)
//                        to single single binary frame
// 
// Cellular Automata tools dialog box handlers
// 
// New dialogs for the CA tools menu should be added to this module.
// The functions that actually perform the actions should be put in
// CA.cpp and CA.h
//
// When making a new dialog start by copying an existing one that is as close to the
// parameter set you will need.
//
#include "framework.h"
#include "MySETIBCA.h"
#include <libloaderapi.h>
#include <Windows.h>
#include <shobjidl.h>
#include <winver.h>
#include <vector>
#include <atlstr.h>
#include <strsafe.h>
#include <atlstr.h>
#include "AppErrors.h"
#include "ImageDialog.h"
#include "Globals.h"
#include "AppFunctions.h"
#include "imageheader.h"
#include "CA.h"
#include "FileFunctions.h"
#include "shellapi.h"

#define BINARY_THRESHOLD 20
void CollapseImageFrames(int* TheImage, IMAGINGHEADER* BCAimageHeader);

// Add new callback prototype declarations in my MySETIBCA.cpp

//*******************************************************************************
//
// Message handler for ReorderImageDlg dialog box.
// 
//*******************************************************************************
INT_PTR CALLBACK MargolusBCADlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        WCHAR szString[MAX_PATH];

    case WM_INITDIALOG:
    {
        IMAGINGHEADER ImageHeader;

        GetPrivateProfileString(L"MargolusBCADlg", L"ImageInput", L"Message.raw", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_IMAGE_INPUT, szString);
        if (ReadImageHeader(szString, &ImageHeader) == 1) {
            SetDlgItemInt(hDlg, IDC_XSIZEI, ImageHeader.Xsize, TRUE);
            SetDlgItemInt(hDlg, IDC_YSIZEI, ImageHeader.Ysize, TRUE);
            SetDlgItemInt(hDlg, IDC_NUM_FRAMES, ImageHeader.NumFrames, TRUE);
        }
        else {
            SetDlgItemInt(hDlg, IDC_XSIZEI, 0, TRUE);
            SetDlgItemInt(hDlg, IDC_YSIZEI, 0, TRUE);
            SetDlgItemInt(hDlg, IDC_NUM_FRAMES, 0, TRUE);
        }

        GetPrivateProfileString(L"MargolusBCADlg", L"TextInput1", L"C:\\MySETIBCA\\Data\\Rules\\single point cw.txt", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_TEXT_INPUT1, szString);

        GetPrivateProfileString(L"MargolusBCADlg", L"TextInput2", L"C:\\MySETIBCA\\Data\\Rules\\single point ccw.txt", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_TEXT_INPUT2, szString);

        GetPrivateProfileString(L"MargolusBCADlg", L"ImageOutput", L"C:\\MySETIBCA\\Data\\Results\\BCAresult.raw", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_IMAGE_OUTPUT, szString);

        GetPrivateProfileString(L"MargolusBCADlg", L"ForwardLimit", L"6625", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_FORWARD_LIMIT, szString);
        
        GetPrivateProfileString(L"MargolusBCADlg", L"BackwardLimit", L"0", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_BACKWARD_LIMIT, szString);

        GetPrivateProfileString(L"MargolusBCADlg", L"ForwardSteps", L"20", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_FORWARD_STEPS, szString);

        GetPrivateProfileString(L"MargolusBCADlg", L"BackwardSteps", L"20", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_BACKWARD_STEPS, szString);

        GetPrivateProfileString(L"MargolusBCADlg", L"FPSrun", L"100", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_FPS_RUN, szString);

        SetDlgItemText(hDlg, IDC_CURRENT_ITERATION, L"0");
        
        CurrentIteration = 0;
        BCAimageLoaded = FALSE;

        int ResetWindows = GetPrivateProfileInt(L"GlobalSettings", L"ResetWindows", 0, (LPCTSTR)strAppNameINI);
        if (!ResetWindows) {
            CString csString = L"MargolusBCAwindow";
            RestoreWindowPlacement(hDlg, csString);
        }

        return (INT_PTR)TRUE;
    }

    case WM_DESTROY:
    {
        // must destroy any brushes created

        if (TheImage) {
            delete[] TheImage;
            TheImage = NULL;
        }
        hwndMargolusBCA = NULL;
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_IMAGE_INPUT_BROWSE:
        {
            PWSTR pszFilename;
            IMAGINGHEADER ImageHeader;

            GetDlgItemText(hDlg, IDC_IMAGE_INPUT, szString, MAX_PATH);
            COMDLG_FILTERSPEC rawType[] =
            {
                 { L"text files", L"*.raw" },
                 { L"All Files", L"*.*" },
            };
            if (!CCFileOpen(hDlg, szString, &pszFilename, FALSE, 2, rawType, L"*.raw")) {
                return (INT_PTR)TRUE;
            }
            {
                wcscpy_s(szString, pszFilename);
                CoTaskMemFree(pszFilename);
            }
            SetDlgItemText(hDlg, IDC_IMAGE_INPUT, szString);

            if (ReadImageHeader(szString, &ImageHeader) == 1) {
                SetDlgItemInt(hDlg, IDC_XSIZEI, ImageHeader.Xsize, TRUE);
                SetDlgItemInt(hDlg, IDC_YSIZEI, ImageHeader.Ysize, TRUE);
                SetDlgItemInt(hDlg, IDC_NUM_FRAMES, ImageHeader.NumFrames, TRUE);
                // ??? warning about multiframe files, only the first frame in file is used
            }
            else {
                SetDlgItemInt(hDlg, IDC_XSIZEI, 0, TRUE);
                SetDlgItemInt(hDlg, IDC_YSIZEI, 0, TRUE);
                SetDlgItemInt(hDlg, IDC_NUM_FRAMES, 0, TRUE);
                MessageBox(hDlg, L"Selected file is not an image file", L"File incompatible", MB_OK);
            }

            return (INT_PTR)TRUE;
        }

        case IDC_TEXT_INPUT1_BROWSE:
        {
            PWSTR pszFilename;
            GetDlgItemText(hDlg, IDC_TEXT_INPUT1, szString, MAX_PATH);
            COMDLG_FILTERSPEC textType[] =
            {
                 { L"text files", L"*.txt" },
                 { L"All Files", L"*.*" },
            };
            if (!CCFileOpen(hDlg, szString, &pszFilename, FALSE, 2, textType, L"*.txt")) {
                return (INT_PTR)TRUE;
            }
            {
                wcscpy_s(szString, pszFilename);
                CoTaskMemFree(pszFilename);
            }
            SetDlgItemText(hDlg, IDC_TEXT_INPUT1, szString);
            return (INT_PTR)TRUE;
        }

        case IDC_TEXT_INPUT2_BROWSE:
        {
            PWSTR pszFilename;
            GetDlgItemText(hDlg, IDC_TEXT_INPUT1, szString, MAX_PATH);
            COMDLG_FILTERSPEC textType[] =
            {
                 { L"text files", L"*.txt" },
                 { L"All Files", L"*.*" },
            };
            if (!CCFileOpen(hDlg, szString, &pszFilename, FALSE, 2, textType, L"*.txt")) {
                return (INT_PTR)TRUE;
            }
            {
                wcscpy_s(szString, pszFilename);
                CoTaskMemFree(pszFilename);
            }
            SetDlgItemText(hDlg, IDC_TEXT_INPUT2, szString);
            return (INT_PTR)TRUE;
        }

        case IDC_IMAGE_OUTPUT_BROWSE:
        {
            PWSTR pszFilename;
            GetDlgItemText(hDlg, IDC_IMAGE_OUTPUT, szString, MAX_PATH);
            COMDLG_FILTERSPEC rawType[] =
            {
                 { L"text files", L"*.raw" },
                 { L"All Files", L"*.*" },
            };
            if (!CCFileSave(hDlg, szString, &pszFilename, FALSE, 2, rawType, L"*.raw")) {
                return (INT_PTR)TRUE;
            }
            {
                wcscpy_s(szString, pszFilename);
                CoTaskMemFree(pszFilename);
            }
            SetDlgItemText(hDlg, IDC_IMAGE_OUTPUT, szString);
            return (INT_PTR)TRUE;
        }

        case IDC_STEP_BACKWARD:
        {
            BOOL bSuccess;
            int BackwardLimit;
            int NumberSteps;

            if (!BCAimageLoaded) {
                // This really shouldn't ever get here but just in case
                return (INT_PTR)TRUE;
            }

            // IDC_BACKWARD_STEPS
            NumberSteps = GetDlgItemInt(hDlg, IDC_BACKWARD_STEPS, &bSuccess, TRUE);
            if (!bSuccess) {
                MessageBox(hDlg, L"Backward steps not valid", L"Not a number", MB_OK);
                return (INT_PTR)TRUE;
            }
            if (NumberSteps < 1) {
                MessageBox(hDlg, L"Backward step size must be >= 1", L"Invalid step size", MB_OK);
                return (INT_PTR)TRUE;
            }

            // read backward limit
            BackwardLimit = GetDlgItemInt(hDlg, IDC_BACKWARD_LIMIT, &bSuccess, TRUE);
            if (!bSuccess) {
                MessageBox(hDlg, L"Backward iteration limit not valid", L"Not a number", MB_OK);
                return (INT_PTR)TRUE;
            }

            // if step at limit we are done
            if (CurrentIteration <= BackwardLimit) {
                return (INT_PTR)TRUE;
            }

            for (int i = 0; i < NumberSteps; i++) {
                // step backward on iteration
                EvenStep = !EvenStep;
                MargolusBCAp1p1(EvenStep, TheImage,
                    BCAimageHeader.Xsize, BCAimageHeader.Ysize, BackwardRules);
                CurrentIteration--;
                if (CurrentIteration <= BackwardLimit) break;
            }

            // update current iteration
            SetDlgItemInt(hDlg, IDC_CURRENT_ITERATION, CurrentIteration, TRUE);

            // update displays
            SendMessage(hwndLayers, WM_COMMAND, ID_UPDATE, 1); // apply 

            return (INT_PTR)TRUE;
        }

        case IDC_STEP_FORWARD:
        {
            BOOL bSuccess;
            int NumberSteps;
            int ForwardLimit;

            if (!BCAimageLoaded) {
                // This really shouldn't ever get here but just in case
                return (INT_PTR)TRUE;
            }

            // IDC_FORWARD_STEPS
            NumberSteps = GetDlgItemInt(hDlg, IDC_FORWARD_STEPS, &bSuccess, TRUE);
            if (!bSuccess) {
                MessageBox(hDlg, L"Forward steps not valid", L"Not a number", MB_OK);
                return (INT_PTR)TRUE;
            }
            if (NumberSteps < 1) {
                MessageBox(hDlg, L"Forward step size must be >= 1", L"Invalid step size", MB_OK);
                return (INT_PTR)TRUE;
            }

            // read forward limit
            ForwardLimit = GetDlgItemInt(hDlg, IDC_FORWARD_LIMIT, &bSuccess, TRUE);
            if (!bSuccess) {
                MessageBox(hDlg, L"Forward iteration limit not valid", L"Not a number", MB_OK);
                return (INT_PTR)TRUE;
            }

            // if step at limit we are done
            if (CurrentIteration >= ForwardLimit) {
                return (INT_PTR)TRUE;
            }

            for (int i = 0; i < NumberSteps; i++) {
                // step forward on iteration
                MargolusBCAp1p1(EvenStep, TheImage,
                    BCAimageHeader.Xsize, BCAimageHeader.Ysize, ForwardRules);

                CurrentIteration++;
                EvenStep = !EvenStep;
                if (CurrentIteration >= ForwardLimit) break;
            }

            // update current iteration
            SetDlgItemInt(hDlg, IDC_CURRENT_ITERATION, CurrentIteration, TRUE);
            
            // update displays
            SendMessage(hwndLayers, WM_COMMAND, ID_UPDATE, 1); // apply 

            return (INT_PTR)TRUE;
        }

        case IDC_RUN_FORWARD:
        {
            BOOL bSuccess;
            int ForwardLimit;
            int FPArun;

            if (!BCAimageLoaded) {
                // This really shouldn't ever get here but just in case
                return (INT_PTR)TRUE;
            }

            if (BCArunning == 1) {
                return (INT_PTR)TRUE;
            }
            else if (BCArunning == -1) {
                // timer code does the rest
                BCArunning = 1;
                return (INT_PTR)TRUE;
            }

            // read frames per second to run
            FPArun = GetDlgItemInt(hDlg, IDC_FPS_RUN, &bSuccess, TRUE);
            if (!bSuccess) {
                MessageBox(hDlg, L"Bad fps number", L"Not a number", MB_OK);
                return (INT_PTR)TRUE;
            }
            if (FPArun < 1 || FPArun>100) {
                MessageBox(hDlg, L"fps must be in range 1-100", L"out of range", MB_OK);
                return (INT_PTR)TRUE;
            }

            // read forward limit
            ForwardLimit = GetDlgItemInt(hDlg, IDC_FORWARD_LIMIT, &bSuccess, TRUE);
            if (!bSuccess) {
                MessageBox(hDlg, L"Forward iteration limit not valid", L"Not a number", MB_OK);
                return (INT_PTR)TRUE;
            }

            if (CurrentIteration >= ForwardLimit) {
                return (INT_PTR)TRUE;
            }

            BCArunning = 1;
            
            HWND ItemHandle = GetDlgItem(hDlg, IDC_STOP);
            EnableWindow(ItemHandle, TRUE);

            ItemHandle = GetDlgItem(hDlg, IDC_STEP_BACKWARD);
            EnableWindow(ItemHandle, FALSE);

            ItemHandle = GetDlgItem(hDlg, IDC_STEP_FORWARD);
            EnableWindow(ItemHandle, FALSE);

            ItemHandle = GetDlgItem(hDlg, IDC_FORWARD_LIMIT);
            EnableWindow(ItemHandle, FALSE);

            ItemHandle = GetDlgItem(hDlg, IDC_BACKWARD_LIMIT);
            EnableWindow(ItemHandle, FALSE);

            ItemHandle = GetDlgItem(hDlg, IDC_FORWARD_STEPS);
            EnableWindow(ItemHandle, FALSE);

            ItemHandle = GetDlgItem(hDlg, IDC_BACKWARD_STEPS);
            EnableWindow(ItemHandle, FALSE);

            // create timer
            UINT Ticks = 1000 / FPArun;

            SetTimer(hwndMain, IDT_BCA_RUN_TIMER, Ticks, (TIMERPROC)NULL);

            return (INT_PTR)TRUE;
        }

        case IDC_RUN_BACKWARD:
        {
            BOOL bSuccess;
            int BackwardLimit;
            int FPArun;

            if (!BCAimageLoaded) {
                // This really shouldn't ever get here but just in case
                return (INT_PTR)TRUE;
            }

            if (BCArunning == -1) {
                return (INT_PTR)TRUE;
            }
            else if (BCArunning == 1) {
                // timer code does the rest
                BCArunning = -1;
                return (INT_PTR)TRUE;
            }

            // read frames per second to run
            FPArun = GetDlgItemInt(hDlg, IDC_FPS_RUN, &bSuccess, TRUE);
            if (!bSuccess) {
                MessageBox(hDlg, L"Bad fps number", L"Not a number", MB_OK);
                return (INT_PTR)TRUE;
            }
            if (FPArun < 1 || FPArun>100) {
                MessageBox(hDlg, L"fps must be in range 1-100", L"out of range", MB_OK);
                return (INT_PTR)TRUE;
            }

            // read forward limit
            BackwardLimit = GetDlgItemInt(hDlg, IDC_BACKWARD_LIMIT, &bSuccess, TRUE);
            if (!bSuccess) {
                MessageBox(hDlg, L"Forward iteration limit not valid", L"Not a number", MB_OK);
                return (INT_PTR)TRUE;
            }

            if (CurrentIteration <= BackwardLimit) {
                return (INT_PTR)TRUE;
            }

            BCArunning = -1;

            HWND ItemHandle = GetDlgItem(hDlg, IDC_STOP);
            EnableWindow(ItemHandle, TRUE);

            ItemHandle = GetDlgItem(hDlg, IDC_STEP_BACKWARD);
            EnableWindow(ItemHandle, FALSE);

            ItemHandle = GetDlgItem(hDlg, IDC_STEP_FORWARD);
            EnableWindow(ItemHandle, FALSE);

            ItemHandle = GetDlgItem(hDlg, IDC_FORWARD_LIMIT);
            EnableWindow(ItemHandle, FALSE);

            ItemHandle = GetDlgItem(hDlg, IDC_BACKWARD_LIMIT);
            EnableWindow(ItemHandle, FALSE);
            
            ItemHandle = GetDlgItem(hDlg, IDC_FORWARD_STEPS);
            EnableWindow(ItemHandle, FALSE);

            ItemHandle = GetDlgItem(hDlg, IDC_BACKWARD_STEPS);
            EnableWindow(ItemHandle, FALSE);

            // create timer
            UINT Ticks = 1000 / FPArun;

            SetTimer(hwndMain, IDT_BCA_RUN_TIMER, Ticks, (TIMERPROC)NULL);

            return (INT_PTR)TRUE;
        }

        case IDC_RELOAD:
        {
            int iRes;
            WCHAR InputFile[MAX_PATH];

            if(BCAimageLoaded) {
                delete[] TheImage;
                TheImage = nullptr;
                BCAimageLoaded = FALSE;
            }

            // Disable Layer 0 in Display dialog
            ImageLayers->DisableLayer(0);

            // read forward rules file
            GetDlgItemText(hDlg, IDC_TEXT_INPUT1, InputFile, MAX_PATH);
            iRes = ReadRulesFile(hDlg, InputFile, ForwardRules);
            if (iRes != APP_SUCCESS) {
                if (iRes == APPERR_PARAMETER) {
                    MessageMySETIBCAError(hDlg, iRes, L"Rules are not 0-15");
                }
                else {
                    MessageMySETIBCAError(hDlg, iRes, L"Reading forward rules file");
                }
                return (INT_PTR)TRUE;
            }

            // read backward rules file
            GetDlgItemText(hDlg, IDC_TEXT_INPUT2, InputFile, MAX_PATH);
            iRes = ReadRulesFile(hDlg, InputFile, BackwardRules);
            if (iRes != APP_SUCCESS) {
                if (iRes == APPERR_PARAMETER) {
                    MessageMySETIBCAError(hDlg, iRes, L"Rules are not 0-15");
                }
                else {
                    MessageMySETIBCAError(hDlg, iRes, L"Reading forward rules file");
                }
                return (INT_PTR)TRUE;
            }

            CurrentIteration = 0;
            EvenStep = TRUE;
            GetDlgItemText(hDlg, IDC_IMAGE_INPUT, InputFile, MAX_PATH);

            // read image file into memory
            iRes = LoadImageFile(&TheImage, InputFile, &BCAimageHeader);
            if (iRes != APP_SUCCESS) {
                MessageBox(hDlg, L"Could not load input image, check format", L"File I/O error", MB_OK);
                return (INT_PTR)TRUE;
            }

            // the image must be even in xsize and ysize
            if (((BCAimageHeader.Xsize % 2)!=0) || ((BCAimageHeader.Ysize % 2)!=0)) {
                delete[] TheImage;
                TheImage = nullptr;
                MessageBox(hDlg, L"Image not loaded, x,y sizes must be even", L"File size error", MB_OK);
                return (INT_PTR)TRUE;
            }
            // check if this is 3 frame image (3 frame raw files are used as color images)
            // If it is convert the 3 frames into the first frame as a binary 0 or 255
            // handling of color images is possible in future update
            if (BCAimageHeader.NumFrames == 3) {
                CollapseImageFrames(TheImage, &BCAimageHeader);
            }

            // update Layer 0 in display dialog
            ImageLayers->UpdateLayer(0, InputFile, TheImage, BCAimageHeader.Xsize, BCAimageHeader.Ysize);

            // enable save button, step forward, step backward
            HWND ItemHandle;

            ItemHandle = GetDlgItem(hDlg, IDC_STEP_BACKWARD);
            EnableWindow(ItemHandle, TRUE);

            ItemHandle = GetDlgItem(hDlg, IDC_RUN_BACKWARD);
            EnableWindow(ItemHandle, TRUE);

            ItemHandle = GetDlgItem(hDlg, IDC_STEP_FORWARD);
            EnableWindow(ItemHandle, TRUE);

            ItemHandle = GetDlgItem(hDlg, IDC_RUN_FORWARD);
            EnableWindow(ItemHandle, TRUE);

            ItemHandle = GetDlgItem(hDlg, IDC_SAVE_IMAGE);
            EnableWindow(ItemHandle, TRUE);

            SetDlgItemInt(hDlg, IDC_CURRENT_ITERATION, CurrentIteration, TRUE);
            BCAimageLoaded = TRUE;

            // update displays
            SendMessage(hwndLayers, WM_COMMAND, ID_UPDATE, 1); // apply 

            return (INT_PTR)TRUE;
        }

        case IDC_SAVE_IMAGE:
        {
            if (BCAimageLoaded) {
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

                swprintf_s(NewFname, _MAX_FNAME, L"%s_%d", Fname, CurrentIteration);

                // reassemble filename
                err = _wmakepath_s(NewFilename, _MAX_PATH, Drive, Dir, NewFname, Ext);
                if (err != 0) {
                    MessageBox(hDlg, L"Could not creat output filename", L"BCA save image", MB_OK);
                    return APPERR_FILEOPEN;
                }

                int iRes;
                iRes = SaveImageFile(hDlg, TheImage, NewFilename, &BCAimageHeader);
            }
            return (INT_PTR)TRUE;
        }

        case IDC_STOP:
        {
            HWND ItemHandle = GetDlgItem(hDlg, IDC_STOP);
            EnableWindow(ItemHandle, TRUE);

            ItemHandle = GetDlgItem(hDlg, IDC_STEP_BACKWARD);
            EnableWindow(ItemHandle, TRUE);

            ItemHandle = GetDlgItem(hDlg, IDC_STEP_FORWARD);
            EnableWindow(ItemHandle, TRUE);

            ItemHandle = GetDlgItem(hDlg, IDC_FORWARD_LIMIT);
            EnableWindow(ItemHandle, TRUE);

            ItemHandle = GetDlgItem(hDlg, IDC_BACKWARD_LIMIT);
            EnableWindow(ItemHandle, TRUE);

            ItemHandle = GetDlgItem(hDlg, IDC_FORWARD_STEPS);
            EnableWindow(ItemHandle, TRUE);

            ItemHandle = GetDlgItem(hDlg, IDC_BACKWARD_STEPS);
            EnableWindow(ItemHandle, TRUE);

            KillTimer(hwndMain, IDT_BCA_RUN_TIMER);
            BCArunning = 0;
            return (INT_PTR)TRUE;
        }

        case IDOK:
        {
            GetDlgItemText(hDlg, IDC_IMAGE_INPUT, szString, MAX_PATH);
            WritePrivateProfileString(L"MargolusBCADlg", L"ImageInput", szString, (LPCTSTR)strAppNameINI);

            GetDlgItemText(hDlg, IDC_TEXT_INPUT1, szString, MAX_PATH);
            WritePrivateProfileString(L"MargolusBCADlg", L"TextInput1", szString, (LPCTSTR)strAppNameINI);

            GetDlgItemText(hDlg, IDC_TEXT_INPUT2, szString, MAX_PATH);
            WritePrivateProfileString(L"MargolusBCADlg", L"TextInput2", szString, (LPCTSTR)strAppNameINI);

            GetDlgItemText(hDlg, IDC_IMAGE_OUTPUT, szString, MAX_PATH);
            WritePrivateProfileString(L"MargolusBCADlg", L"ImageOutput", szString, (LPCTSTR)strAppNameINI);

            GetDlgItemText(hDlg, IDC_BACKWARD_LIMIT, szString, MAX_PATH);
            WritePrivateProfileString(L"MargolusBCADlg", L"BackwardLimit", szString, (LPCTSTR)strAppNameINI);

            GetDlgItemText(hDlg, IDC_FORWARD_LIMIT, szString, MAX_PATH);
            WritePrivateProfileString(L"MargolusBCADlg", L"ForwardLimit", szString, (LPCTSTR)strAppNameINI);

            GetDlgItemText(hDlg, IDC_FORWARD_STEPS, szString, MAX_PATH);
            WritePrivateProfileString(L"MargolusBCADlg", L"ForwardSteps", szString, (LPCTSTR)strAppNameINI);

            GetDlgItemText(hDlg, IDC_BACKWARD_STEPS, szString, MAX_PATH);
            WritePrivateProfileString(L"MargolusBCADlg", L"BackwardSteps", szString, (LPCTSTR)strAppNameINI);

            GetDlgItemText(hDlg, IDC_FPS_RUN, szString, MAX_PATH);
            WritePrivateProfileString(L"MargolusBCADlg", L"FPSrun", szString, (LPCTSTR)strAppNameINI);

            {
                // save window position/size data
                CString csString = L"MargolusBCAwindow";
                SaveWindowPlacement(hDlg, csString);
            }
            ShowWindow(hDlg, SW_HIDE);
            return (INT_PTR)TRUE;
        }
        case IDCANCEL:
        {
            ShowWindow(hDlg, SW_HIDE);
            return (INT_PTR)TRUE;
        }

        } // end of WM_COMMAND case switch
    
    }
    return (INT_PTR)FALSE;
}

//*******************************************************************************
//
// CollapseImageFrames
// 
// This is to convert color image from 3 frame raw file to single frame binary 0/255
// Sum all 3 frames into the first frame.
// Use Threshold to binarize
// 
// int* TheImage
// IMAGINGHEADER* BCAimageHeader
// 
// no return parameter
// 
//*******************************************************************************
void CollapseImageFrames(int* TheImage, IMAGINGHEADER* BCAimageHeader) {
    int FrameOffset;
    int x, y;
    int Offset;

    FrameOffset = BCAimageHeader->Xsize * BCAimageHeader->Ysize;

    for (y = 0, Offset = 0; y < BCAimageHeader->Ysize; y++) {
        for (x = 0; x < BCAimageHeader->Xsize; x++) {
            TheImage[Offset + x] = TheImage[Offset + x] + TheImage[FrameOffset + Offset + x] +
                TheImage[2 * FrameOffset + Offset + x];
            if (TheImage[Offset + x] < BINARY_THRESHOLD) {
                TheImage[Offset + x] = 0;
            }
            else {
                TheImage[Offset + x] = 255;
            }
        }
        Offset += BCAimageHeader->Xsize;
    }
    
    return;
}