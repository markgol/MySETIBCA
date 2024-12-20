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
// V1.1.0   2024-06-28  Added ASIS receive/send message dialogs
//                          Recieve reads the ASIS .bin bitstream file and saves a decoded
//                          .raw image file and a .bmp file
//                          Send will encode a .raw image file or .bmp file that is 256x256
//                          and save it as a ASIS bitstream file.  (pixels that have a 
//                          monochromatic or chromatic brightness <50 are considered 0,
//                          >= 50 are considered 1)
//                      Added even/odd controls to Margolus BCA, this shows wether the next
//                          iteration will be an even (0,0) or odd grid (1,1).  Also allows
//                          user to change the current selelction.  Note:  ASIS uses an even
//                          starting grid.
//                      Allow also the use of .bmp file instead of .raw file
//                      Automatically save of BMP and (optionally) PNG file when image file is saved
// V1.1.2   2024-07-10  Added histogram of # bits set in 2x2 block for image
//                      Added current non zero bit count to BCA dialog
//                      Send ASIS message saves the number of iterations as default for next time
//                      Allow override of iterations specfied in the Receive ASIS message
//                      Allow generation of footer based on # iterations selected rather than use a file
//                      Correction, removed bit order selection from send and receive ASIS message
//                          We know what order ASIS used and the compiler was incorrectly optimizing
//                          the selection criteria with different result for Debug and and Release 
//                      Correction, filename extension  for send message corrected to default of .bin
//                      Correction, default filename for footer was the header file
//                      Correction, Receive ASIS message starting EvenStep = FALSE when # of iterations is even
// V1.1.5   2024-11-08  Added save snapshot to each iteration.
// V1.1.7   2024-12-05  Added Unary sequence dialog
//                      Records possible unary sequences of factors for possible iterations for the BCA which have
//                      a specific last unary value in a fixed bit string length.
// V1.1.8   2024-12-18  Corrected errors in output file for unary calculations
//                      Added Generic Finite State Machine dialog
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
#include <cmath>
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
#include "GenericFSM.h"

BOOL NewHistoFile = TRUE;
GenericFSM* MyFSM = nullptr;

void ResetTheFSM(HWND hDlg, BOOL ClearResults);
int ProcessSequenceUsingFSM(HWND hDlg, WCHAR* Sequence, size_t MaxSeqLength);

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
        // check if .raw image file format first
        if (ReadImageHeader(szString, &ImageHeader) == APP_SUCCESS) {
            SetDlgItemInt(hDlg, IDC_XSIZEI, ImageHeader.Xsize, TRUE);
            SetDlgItemInt(hDlg, IDC_YSIZEI, ImageHeader.Ysize, TRUE);
            SetDlgItemInt(hDlg, IDC_NUM_FRAMES, ImageHeader.NumFrames, TRUE);
        }
        else {
            // then try .bmp file format
            int* InputImage;
            if (ReadBMPfile(&InputImage, szString, &ImageHeader) == APP_SUCCESS) {
                SetDlgItemInt(hDlg, IDC_XSIZEI, ImageHeader.Xsize, TRUE);
                SetDlgItemInt(hDlg, IDC_YSIZEI, ImageHeader.Ysize, TRUE);
                SetDlgItemInt(hDlg, IDC_NUM_FRAMES, ImageHeader.NumFrames, TRUE);
                delete[] InputImage;
            }
            else {
                // unrecognized file format
                SetDlgItemInt(hDlg, IDC_XSIZEI, 0, TRUE);
                SetDlgItemInt(hDlg, IDC_YSIZEI, 0, TRUE);
                SetDlgItemInt(hDlg, IDC_NUM_FRAMES, 0, TRUE);
            }
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

        GetPrivateProfileString(L"MargolusBCADlg", L"Threshold", L"2", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_THRESHOLD, szString);

        int iRes = GetPrivateProfileInt(L"MargolusBCADlg", L"AutoBMP", 1, (LPCTSTR)strAppNameINI);
        if (iRes != 0) {
            CheckDlgButton(hDlg, IDC_BMP_FILE, BST_CHECKED);
        }

        iRes = GetPrivateProfileInt(L"MargolusBCADlg", L"HistoFileSave", 0, (LPCTSTR)strAppNameINI);
        if (iRes != 0) {
            CheckDlgButton(hDlg, IDC_HISTO_FILE, BST_CHECKED);
        }

        SetDlgItemText(hDlg, IDC_CURRENT_ITERATION, L"0");
        
        EvenStep = TRUE;
        if (EvenStep) {
            CheckRadioButton(hDlg, IDC_EVEN, IDC_ODD, IDC_EVEN);
        }
        else {
            CheckRadioButton(hDlg, IDC_EVEN, IDC_ODD, IDC_ODD);
        }
        
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
                 { L"raw image files", L"*.raw" },
                 { L"Bitmap files", L"*.bmp"},
                 { L"All Files", L"*.*" },
            };
            if (!CCFileOpen(hDlg, szString, &pszFilename, FALSE, 3, rawType, L"*.raw")) {
                return (INT_PTR)TRUE;
            }
            {
                wcscpy_s(szString, pszFilename);
                CoTaskMemFree(pszFilename);
            }
            SetDlgItemText(hDlg, IDC_IMAGE_INPUT, szString);

            // check if .raw image file format first
            if (ReadImageHeader(szString, &ImageHeader) == APP_SUCCESS) {
                SetDlgItemInt(hDlg, IDC_XSIZEI, ImageHeader.Xsize, TRUE);
                SetDlgItemInt(hDlg, IDC_YSIZEI, ImageHeader.Ysize, TRUE);
                SetDlgItemInt(hDlg, IDC_NUM_FRAMES, ImageHeader.NumFrames, TRUE);
            }
            else {
                // then try .bmp file format
                int* InputImage;
                if (ReadBMPfile(&InputImage, szString, &ImageHeader) == APP_SUCCESS) {
                    SetDlgItemInt(hDlg, IDC_XSIZEI, ImageHeader.Xsize, TRUE);
                    SetDlgItemInt(hDlg, IDC_YSIZEI, ImageHeader.Ysize, TRUE);
                    SetDlgItemInt(hDlg, IDC_NUM_FRAMES, ImageHeader.NumFrames, TRUE);
                    delete[] InputImage;
                }
                else {
                    // unrecognized file format
                    SetDlgItemInt(hDlg, IDC_XSIZEI, 0, TRUE);
                    SetDlgItemInt(hDlg, IDC_YSIZEI, 0, TRUE);
                    SetDlgItemInt(hDlg, IDC_NUM_FRAMES, 0, TRUE);
                }
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

        case IDC_EVEN:
        {
            if (IsDlgButtonChecked(hDlg, IDC_EVEN)) {
                EvenStep = TRUE;
            }
            return (INT_PTR)TRUE;
        }

        case IDC_ODD:
        {
             if (IsDlgButtonChecked(hDlg, IDC_ODD)) {
                EvenStep = FALSE;
            }
            return (INT_PTR)TRUE;
        }
        case IDC_STEP_BACKWARD:
        {
            BOOL bSuccess;
            int BackwardLimit;
            int NumberSteps;
            int Histo[5] = { 0,0,0,0,0 };
            BOOL HistoFileSave = FALSE;
            BOOL SaveStep = FALSE;

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

            if (IsDlgButtonChecked(hDlg, IDC_HISTO_FILE) == BST_CHECKED) {
                HistoFileSave = TRUE;
            }

            if (IsDlgButtonChecked(hDlg, IDC_SAVE_STEP) == BST_CHECKED) {
                SaveStep = TRUE;
            }

            for (int i = 0; i < NumberSteps; i++) {
                // step backward on iteration
                EvenStep = !EvenStep;
                Histo[0] = 0;
                Histo[1] = 0;
                Histo[2] = 0;
                Histo[3] = 0;
                Histo[4] = 0;
                MargolusBCAp1p1(EvenStep, TheImage,
                    BCAimageHeader.Xsize, BCAimageHeader.Ysize,
                    BackwardRules, Histo);
                CurrentIteration--;
                if (HistoFileSave) {
                    WCHAR Filename[MAX_PATH];
                    GetDlgItemText(hDlg, IDC_IMAGE_OUTPUT, Filename, MAX_PATH);
                    SaveHistogramData(Filename, NewHistoFile, CurrentIteration, Histo, 5);
                    NewHistoFile = FALSE;
                }

                if (CurrentIteration <= BackwardLimit) break;
            }

            if (SaveStep) {
                SaveSnapshot(hDlg, CurrentIteration, TheImage, &BCAimageHeader);
            }

            // update bit count
            int Count = CountBitInImage(TheImage, &BCAimageHeader);
            SetDlgItemInt(hDlg, IDC_CURRENT_BITS, Count, TRUE);

            // update current iteration
            SetDlgItemInt(hDlg, IDC_CURRENT_ITERATION, CurrentIteration, TRUE);
            
            // update step radio buttons
            if (EvenStep) {
                CheckRadioButton(hDlg, IDC_EVEN, IDC_ODD, IDC_EVEN);
            }
            else {
                CheckRadioButton(hDlg, IDC_EVEN, IDC_ODD, IDC_ODD);
            }
            // update histogram data
            SetDlgItemInt(hDlg, IDC_HISTO0, Histo[0], TRUE);
            SetDlgItemInt(hDlg, IDC_HISTO1, Histo[1], TRUE);
            SetDlgItemInt(hDlg, IDC_HISTO2, Histo[2], TRUE);
            SetDlgItemInt(hDlg, IDC_HISTO3, Histo[3], TRUE);
            SetDlgItemInt(hDlg, IDC_HISTO4, Histo[4], TRUE);

            // update displays
            SendMessage(hwndLayers, WM_COMMAND, ID_UPDATE, 1); // apply 

            return (INT_PTR)TRUE;
        }

        case IDC_STEP_FORWARD:
        {
            BOOL bSuccess;
            int NumberSteps;
            int ForwardLimit;
            int Histo[5] = { 0,0,0,0,0 };
            BOOL HistoFileSave = FALSE;
            BOOL SaveStep = FALSE;

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

            if (IsDlgButtonChecked(hDlg, IDC_HISTO_FILE) == BST_CHECKED) {
                HistoFileSave = TRUE;
            }

            if (IsDlgButtonChecked(hDlg, IDC_SAVE_STEP) == BST_CHECKED) {
                SaveStep = TRUE;
            }

            for (int i = 0; i < NumberSteps; i++) {
                Histo[0] = 0;
                Histo[1] = 0;
                Histo[2] = 0;
                Histo[3] = 0;
                Histo[4] = 0;

                // step forward on iteration
                MargolusBCAp1p1(EvenStep, TheImage,
                    BCAimageHeader.Xsize, BCAimageHeader.Ysize,
                    ForwardRules, Histo);

                CurrentIteration++;
                if (HistoFileSave) {
                    WCHAR Filename[MAX_PATH];
                    GetDlgItemText(hDlg, IDC_IMAGE_OUTPUT, Filename, MAX_PATH);
                    SaveHistogramData(Filename, NewHistoFile, CurrentIteration, Histo, 5);
                    NewHistoFile = FALSE;
                }

                EvenStep = !EvenStep;
                if (CurrentIteration >= ForwardLimit) break;
            }

            if (SaveStep) {
                SaveSnapshot(hDlg, CurrentIteration, TheImage, &BCAimageHeader);
            }

            // update bit count
            int Count = CountBitInImage(TheImage, &BCAimageHeader);
            SetDlgItemInt(hDlg, IDC_CURRENT_BITS, Count, TRUE);

            // update current iteration
            SetDlgItemInt(hDlg, IDC_CURRENT_ITERATION, CurrentIteration, TRUE);
            
            if (EvenStep) {
                CheckRadioButton(hDlg, IDC_EVEN, IDC_ODD, IDC_EVEN);
            }
            else {
                CheckRadioButton(hDlg, IDC_EVEN, IDC_ODD, IDC_ODD);
            }

            // update histogram data
            SetDlgItemInt(hDlg, IDC_HISTO0, Histo[0], TRUE);
            SetDlgItemInt(hDlg, IDC_HISTO1, Histo[1], TRUE);
            SetDlgItemInt(hDlg, IDC_HISTO2, Histo[2], TRUE);
            SetDlgItemInt(hDlg, IDC_HISTO3, Histo[3], TRUE);
            SetDlgItemInt(hDlg, IDC_HISTO4, Histo[4], TRUE);

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
            NewHistoFile = TRUE;

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
            CheckRadioButton(hDlg, IDC_EVEN, IDC_ODD, IDC_EVEN);

            GetDlgItemText(hDlg, IDC_IMAGE_INPUT, InputFile, MAX_PATH);

            // read image file into memory
            int UseThisThreshold;
            BOOL bSuccess;

            UseThisThreshold = GetDlgItemInt(hDlg, IDC_THRESHOLD, &bSuccess, TRUE);
            if (!bSuccess || UseThisThreshold <= 0) {
                MessageBox(hDlg, L"Invalid threshold or <= 0", L"Bad Number", MB_OK);
                return (INT_PTR)TRUE;
            }

            iRes = LoadImageFile(&TheImage, InputFile, &BCAimageHeader);
            if (iRes != APP_SUCCESS) {
                // then try .bmp format
                iRes = ReadBMPfile(&TheImage, InputFile, &BCAimageHeader);
                if (iRes != APP_SUCCESS) {
                    MessageBox(hDlg, L"Input image file is not valid", L"File read error", MB_OK);
                    return (INT_PTR)TRUE;
                }
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
            if (BCAimageHeader.NumFrames == 3) {
                CollapseImageFrames(TheImage, &BCAimageHeader, UseThisThreshold);
            }
            else {
                BinarizeImage(TheImage, &BCAimageHeader, UseThisThreshold);
            }
            // clear histogram
            SetDlgItemText(hDlg, IDC_HISTO0, L"");
            SetDlgItemText(hDlg, IDC_HISTO1, L"");
            SetDlgItemText(hDlg, IDC_HISTO2, L"");
            SetDlgItemText(hDlg, IDC_HISTO3, L"");
            SetDlgItemText(hDlg, IDC_HISTO4, L"");

            // update bit count
            int Count = CountBitInImage(TheImage, &BCAimageHeader);
            SetDlgItemInt(hDlg, IDC_CURRENT_BITS, Count, TRUE);

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
                SaveSnapshot(hDlg, CurrentIteration, TheImage, &BCAimageHeader);
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

            if (EvenStep) {
                CheckRadioButton(hDlg, IDC_EVEN, IDC_ODD, IDC_EVEN);
            }
            else {
                CheckRadioButton(hDlg, IDC_EVEN, IDC_ODD, IDC_ODD);
            }

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

            GetDlgItemText(hDlg, IDC_THRESHOLD, szString, MAX_PATH);
            WritePrivateProfileString(L"MargolusBCADlg", L"Threshold", szString, (LPCTSTR)strAppNameINI);

            if (IsDlgButtonChecked(hDlg, IDC_BMP_FILE) == BST_CHECKED) {
                WritePrivateProfileString(L"MargolusBCADlg", L"AutoBMP", L"1", (LPCTSTR)strAppNameINI);
            }
            else {
                WritePrivateProfileString(L"MargolusBCADlg", L"AutoBMP", L"0", (LPCTSTR)strAppNameINI);
            }

            if (IsDlgButtonChecked(hDlg, IDC_HISTO_FILE) == BST_CHECKED) {
                WritePrivateProfileString(L"MargolusBCADlg", L"HistoFileSave", L"1", (LPCTSTR)strAppNameINI);
            }
            else {
                WritePrivateProfileString(L"MargolusBCADlg", L"HistoFileSave", L"0", (LPCTSTR)strAppNameINI);
            }

            CheckDlgButton(hDlg, IDC_SAVE_STEP, BST_UNCHECKED);

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
            CheckDlgButton(hDlg, IDC_SAVE_STEP, BST_UNCHECKED);

            ShowWindow(hDlg, SW_HIDE);
            return (INT_PTR)TRUE;
        }

        } // end of WM_COMMAND case switch
    
    }
    return (INT_PTR)FALSE;
}

//*******************************************************************************
//
// Message handler for ReceiveASISdlg dialog box.
// 
//*******************************************************************************
INT_PTR CALLBACK ReceiveASISdlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        WCHAR szString[MAX_PATH];

    case WM_INITDIALOG:
    {
        IMAGINGHEADER ImageHeader;
        int BCAiterations = 0;
        int* InputImage;
        BYTE Header[10];
        BYTE Footer[10];

        GetPrivateProfileString(L"ReceiveASISdlg", L"ImageInput",
            L"C:\\MySETIBCA\\Data\\Data17.bin", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_IMAGE_INPUT, szString);
        // Read header, body, footer
        int iRes;
        int BitCount;
        int Iterations;
        iRes = ReadASISmessage(szString, &ImageHeader, &InputImage, Header, Footer, &Iterations, &BitCount);
        if (iRes == APP_SUCCESS) {
            //IDC_NUM_BCA_STEPS
            SetDlgItemInt(hDlg, IDC_NUM_BCA_STEPS, Iterations, TRUE);
            SetDlgItemInt(hDlg, IDC_NUM_BITS, BitCount, TRUE);
            delete[] InputImage;
        }
        else {
            SetDlgItemInt(hDlg, IDC_NUM_BCA_STEPS, 0, TRUE);
            SetDlgItemInt(hDlg, IDC_NUM_BITS, 0, TRUE);
        }

        GetPrivateProfileString(L"ReceiveASISdlg", L"TextOutput1", L"C:\\MySETIBCA\\Data\\Results\\Header.txt", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_TEXT_OUTPUT1, szString);

        GetPrivateProfileString(L"ReceiveASISdlg", L"TextOutput2", L"C:\\MySETIBCA\\Data\\Results\\Footer.txt", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_TEXT_OUTPUT2, szString);

        GetPrivateProfileString(L"ReceiveASISdlg", L"ImageOutput", L"C:\\MySETIBCA\\Data\\Results\\BCAresult.raw", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_IMAGE_OUTPUT, szString);

        iRes = GetPrivateProfileInt(L"ReceiveASISdlg", L"AutoBMP", 1, (LPCTSTR)strAppNameINI);
        if (iRes != 0) {
            CheckDlgButton(hDlg, IDC_BMP_FILE, BST_CHECKED);
        }

        int ResetWindows = GetPrivateProfileInt(L"GlobalSettings", L"ResetWindows", 0, (LPCTSTR)strAppNameINI);
        if (!ResetWindows) {
            CString csString = L"ReceiveASISdlg";
            RestoreWindowPlacement(hDlg, csString);
        }

        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_IMAGE_INPUT_BROWSE:
        {
            PWSTR pszFilename;
            GetDlgItemText(hDlg, IDC_IMAGE_INPUT, szString, MAX_PATH);
            COMDLG_FILTERSPEC textType[] =
            {
                 { L"ASIS binary files", L"*.bin" },
                 { L"All Files", L"*.*" },
            };
            if (!CCFileOpen(hDlg, szString, &pszFilename, FALSE, 2, textType, L"*.bin")) {
                return (INT_PTR)TRUE;
            }
            {
                wcscpy_s(szString, pszFilename);
                CoTaskMemFree(pszFilename);
            }
            SetDlgItemText(hDlg, IDC_IMAGE_INPUT, szString);

            // Read header, body, footer
            int iRes;
            IMAGINGHEADER ImageHeader;
            int BCAiterations = 0;
            int* InputImage;
            BYTE Header[10];
            BYTE Footer[10];
            int IterationsNeeded;
            int BitCount;

            iRes = ReadASISmessage(szString, &ImageHeader, &InputImage, Header, Footer,
                &IterationsNeeded, &BitCount);
            if (iRes == APP_SUCCESS) {
                //IDC_NUM_BCA_STEPS
                SetDlgItemInt(hDlg, IDC_NUM_BCA_STEPS, IterationsNeeded, TRUE);
                SetDlgItemInt(hDlg, IDC_NUM_BITS, BitCount, TRUE);
                delete[] InputImage;
            }
            else {
                SetDlgItemInt(hDlg, IDC_NUM_BCA_STEPS, 0, TRUE);
                SetDlgItemInt(hDlg, IDC_NUM_BITS, 0, TRUE);
            }

            return (INT_PTR)TRUE;
        }

        case IDC_TEXT_OUTPUT1_BROWSE:
        {
            PWSTR pszFilename;
            GetDlgItemText(hDlg, IDC_TEXT_OUTPUT1, szString, MAX_PATH);
            COMDLG_FILTERSPEC textType[] =
            {
                 { L"text files", L"*.txt" },
                 { L"All Files", L"*.*" },
            };
            if (!CCFileSave(hDlg, szString, &pszFilename, FALSE, 2, textType, L"*.txt")) {
                return (INT_PTR)TRUE;
            }
            {
                wcscpy_s(szString, pszFilename);
                CoTaskMemFree(pszFilename);
            }
            SetDlgItemText(hDlg, IDC_TEXT_OUTPUT1, szString);
            return (INT_PTR)TRUE;
        }

        case IDC_TEXT_OUTPUT2_BROWSE:
        {
            PWSTR pszFilename;
            GetDlgItemText(hDlg, IDC_TEXT_OUTPUT1, szString, MAX_PATH);
            COMDLG_FILTERSPEC textType[] =
            {
                 { L"text files", L"*.txt" },
                 { L"All Files", L"*.*" },
            };
            if (!CCFileSave(hDlg, szString, &pszFilename, FALSE, 2, textType, L"*.txt")) {
                return (INT_PTR)TRUE;
            }
            {
                wcscpy_s(szString, pszFilename);
                CoTaskMemFree(pszFilename);
            }
            SetDlgItemText(hDlg, IDC_TEXT_OUTPUT2, szString);
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

        case IDOK:
        {
            GetDlgItemText(hDlg, IDC_IMAGE_INPUT, szString, MAX_PATH);

            // Convert and save ASIS message
            // 
            // Read ASIS message
            //             // Read header, body, footer
            int iRes;
            IMAGINGHEADER ImageHeader;
            int BCAiterations = 0;
            int* InputImage;
            BYTE Header[10];
            BYTE Footer[10];
            int IterationsNeeded;
            int BitCount;

            int IterationsInFooter;

            // read iterations from dialog control
            BOOL bSuccess;
            IterationsNeeded = GetDlgItemInt(hDlg, IDC_NUM_BCA_STEPS, &bSuccess, TRUE);
            if (!bSuccess) {
                MessageBox(hDlg, L"invalid number of iterations", L"Invalid number", MB_OK);
                return (INT_PTR)TRUE;
            }

            // read input file
            iRes = ReadASISmessage(szString, &ImageHeader, &InputImage, Header, Footer, 
                &IterationsInFooter, &BitCount);
            if (iRes != APP_SUCCESS) {
                MessageBox(hDlg, L"Input file is not an ASIS message", L"Read error", MB_OK);
                return (INT_PTR)TRUE;
            }
            WritePrivateProfileString(L"ReceiveASISdlg", L"ImageInput", szString, (LPCTSTR)strAppNameINI);

            // Save Header in text file
            GetDlgItemText(hDlg, IDC_TEXT_OUTPUT1, szString, MAX_PATH);
            WritePrivateProfileString(L"ReceiveASISdlg", L"TextOutput1", szString, (LPCTSTR)strAppNameINI);
            iRes = SaveBYTEs2Text(szString, Header, 10, FALSE);
            if (iRes != APP_SUCCESS) {
                MessageBox(hDlg, L"failed to write the header text file", L"File open error", MB_OK);
                return (INT_PTR)TRUE;
            }

            // Save Footer in text file
            GetDlgItemText(hDlg, IDC_TEXT_OUTPUT2, szString, MAX_PATH);
            WritePrivateProfileString(L"ReceiveASISdlg", L"TextOutput2", szString, (LPCTSTR)strAppNameINI);
            iRes = SaveBYTEs2Text(szString, Footer, 10, FALSE);
            if (iRes != APP_SUCCESS) {
                MessageBox(hDlg, L"failed to write the header text file", L"File open error", MB_OK);
                return (INT_PTR)TRUE;
            }

            // run BCA to decode
            // single point CW rules for BCA
            int Rules[16] = { 0, 2, 8, 3, 1, 5, 6, 7, 4, 9,10,11,12,13,14,15};
            int Histo[5] = { 0,0,0,0,0 };

            // EvenStep is true if the number of iterations is odd
            // EvenStep is false if the number of iterations is even
            BOOL EvenStep;
            if (IterationsNeeded % 2) {
                EvenStep = TRUE;
            }
            else {
                EvenStep = FALSE;
            }
            
            for (int i = 0; i < IterationsNeeded; i++) {
                // step forward on iteration
                MargolusBCAp1p1(EvenStep, InputImage,
                    ImageHeader.Xsize, ImageHeader.Ysize, Rules, Histo);
                EvenStep = !EvenStep;
            }

            GetDlgItemText(hDlg, IDC_IMAGE_OUTPUT, szString, MAX_PATH);

            iRes = SaveImageFile(hDlg, InputImage, szString, &ImageHeader);
            if (iRes != APP_SUCCESS) {
                MessageBox(hDlg, L"Output image file save failed", L"File write error", MB_OK);
                return (INT_PTR)TRUE;
            }

            WritePrivateProfileString(L"ReceiveASISdlg", L"ImageOutput", szString, (LPCTSTR)strAppNameINI);
            delete[] InputImage;

            if (IsDlgButtonChecked(hDlg, IDC_BMP_FILE) == BST_CHECKED) {
                WritePrivateProfileString(L"ReceiveASISdlg", L"AutoBMP", L"1", (LPCTSTR)strAppNameINI);
            }
            else {
                WritePrivateProfileString(L"ReceiveASISdlg", L"AutoBMP", L"0", (LPCTSTR)strAppNameINI);
            }

            if (IsDlgButtonChecked(hDlg, IDC_BMP_FILE) == BST_CHECKED) {
                WCHAR BMPFilename[MAX_PATH];
                WCHAR Drive[_MAX_DRIVE];
                WCHAR Dir[_MAX_DIR];
                WCHAR Fname[_MAX_FNAME];
                WCHAR Ext[_MAX_EXT];

                // split apart original filename
                int err;
                err = _wsplitpath_s(szString, Drive, _MAX_DRIVE, Dir, _MAX_DIR, Fname,
                    _MAX_FNAME, Ext, _MAX_EXT);
                if (err != 0) {
                    MessageBox(hDlg, L"Could not creat BMP output filename", L"Receive ASIS message", MB_OK);
                    return (INT_PTR)TRUE;
                }

                // reassemble filename as .bmp file
                err = _wmakepath_s(BMPFilename, _MAX_PATH, Drive, Dir, Fname, L".bmp");
                if (err != 0) {
                    MessageBox(hDlg, L"Could not creat output filename", L"Receive ASIS message", MB_OK);
                    return (INT_PTR)TRUE;
                }

                iRes = SaveBMP(BMPFilename, szString, FALSE, TRUE);
                if (iRes == APP_SUCCESS) {
                    iRes = GetPrivateProfileInt(L"SettingsGlobalDlg", L"AutoPNG", 1, (LPCTSTR)strAppNameINI);
                    if (iRes != 0) {
                        SaveBMP2PNG(BMPFilename);
                    }
                }
            }

            {
                // save window position/size data
                CString csString = L"ReceiveASISdlg";
                SaveWindowPlacement(hDlg, csString);
            }

            WCHAR NewMessage[MAX_PATH];
            swprintf_s(NewMessage, MAX_PATH, L"Decode message complete\n%d iterations\n%d bits set in image",
                IterationsNeeded, BitCount);
            MessageBox(hDlg, NewMessage, L"Receive ASIS message", MB_OK);

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        case IDCANCEL:
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        }
    }

    return (INT_PTR)FALSE;
}

//*******************************************************************************
//
// Message handler for SendASISdlg dialog box.
// 
//*******************************************************************************
INT_PTR CALLBACK SendASISdlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        WCHAR szString[MAX_PATH];

    case WM_INITDIALOG:
    {
        IMAGINGHEADER ImageHeader;
        int BCAiterations = 0;

        int iRes = GetPrivateProfileInt(L"SendASISdlg", L"UseIterations", 0, (LPCTSTR)strAppNameINI);
        if (iRes != 0) {
            CheckDlgButton(hDlg, IDC_USE_STEPS, BST_CHECKED);
        }

        GetPrivateProfileString(L"SendASISdlg", L"ImageInput",
            L"C:\\MySETIBCA\\Data\\MyMessage.raw", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_IMAGE_INPUT, szString);
        // check if .raw image file format first
        if (ReadImageHeader(szString, &ImageHeader) == APP_SUCCESS) {
            SetDlgItemInt(hDlg, IDC_XSIZEI, ImageHeader.Xsize, TRUE);
            SetDlgItemInt(hDlg, IDC_YSIZEI, ImageHeader.Ysize, TRUE);
            SetDlgItemInt(hDlg, IDC_NUM_FRAMES, ImageHeader.NumFrames, TRUE);
        }
        else {
            // then try .bmp file format
            int* InputImage;
            if (ReadBMPfile(&InputImage, szString, &ImageHeader) == APP_SUCCESS) {
                SetDlgItemInt(hDlg, IDC_XSIZEI, ImageHeader.Xsize, TRUE);
                SetDlgItemInt(hDlg, IDC_YSIZEI, ImageHeader.Ysize, TRUE);
                SetDlgItemInt(hDlg, IDC_NUM_FRAMES, ImageHeader.NumFrames, TRUE);
                delete[] InputImage;
            }
            else {
                // unrecognized file format
                SetDlgItemInt(hDlg, IDC_XSIZEI, 0, TRUE);
                SetDlgItemInt(hDlg, IDC_YSIZEI, 0, TRUE);
                SetDlgItemInt(hDlg, IDC_NUM_FRAMES, 0, TRUE);
            }
        }

        GetPrivateProfileString(L"SendASISdlg", L"BCAiterations",
            L"6625", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_NUM_BCA_STEPS, szString);

        GetPrivateProfileString(L"SendASISdlg", L"TextInput1", L"C:\\MySETIBCA\\Data\\Results\\Header.txt", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_TEXT_INPUT1, szString);

        GetPrivateProfileString(L"SendASISdlg", L"TextInput2", L"C:\\MySETIBCA\\Data\\Results\\Footer.txt", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_TEXT_INPUT2, szString);

        GetPrivateProfileString(L"SendASISdlg", L"MessageOutput", L"C:\\MySETIBCA\\Data\\Results\\MyMessage.bin", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_IMAGE_OUTPUT, szString);

        GetPrivateProfileString(L"SendASISdlg", L"Threshold", L"2", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_THRESHOLD, szString);

        int ResetWindows = GetPrivateProfileInt(L"GlobalSettings", L"ResetWindows", 0, (LPCTSTR)strAppNameINI);
        if (!ResetWindows) {
            CString csString = L"SendASISdlg";
            RestoreWindowPlacement(hDlg, csString);
        }

        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_IMAGE_INPUT_BROWSE:
        {
            PWSTR pszFilename;
            GetDlgItemText(hDlg, IDC_IMAGE_INPUT, szString, MAX_PATH);
            COMDLG_FILTERSPEC textType[] =
            {
                 { L"Image files", L"*.raw" },
                { L"Bitmap file", L"*.bmp"},
                 { L"All Files", L"*.*" },
            };
            if (!CCFileOpen(hDlg, szString, &pszFilename, FALSE, 3, textType, L"*.raw")) {
                return (INT_PTR)TRUE;
            }
            {
                wcscpy_s(szString, pszFilename);
                CoTaskMemFree(pszFilename);
            }
            SetDlgItemText(hDlg, IDC_IMAGE_INPUT, szString);
            
            IMAGINGHEADER ImageHeader;
            // check if .raw image file format first
            if (ReadImageHeader(szString, &ImageHeader) == APP_SUCCESS) {
                SetDlgItemInt(hDlg, IDC_XSIZEI, ImageHeader.Xsize, TRUE);
                SetDlgItemInt(hDlg, IDC_YSIZEI, ImageHeader.Ysize, TRUE);
                SetDlgItemInt(hDlg, IDC_NUM_FRAMES, ImageHeader.NumFrames, TRUE);
            }
            else {
                // then try .bmp file format
                int* InputImage;
                if (ReadBMPfile(&InputImage, szString, &ImageHeader) == APP_SUCCESS) {
                    SetDlgItemInt(hDlg, IDC_XSIZEI, ImageHeader.Xsize, TRUE);
                    SetDlgItemInt(hDlg, IDC_YSIZEI, ImageHeader.Ysize, TRUE);
                    SetDlgItemInt(hDlg, IDC_NUM_FRAMES, ImageHeader.NumFrames, TRUE);
                    delete[] InputImage;
                }
                else {
                    // unrecognized file format
                    SetDlgItemInt(hDlg, IDC_XSIZEI, 0, TRUE);
                    SetDlgItemInt(hDlg, IDC_YSIZEI, 0, TRUE);
                    SetDlgItemInt(hDlg, IDC_NUM_FRAMES, 0, TRUE);
                }
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
            GetDlgItemText(hDlg, IDC_TEXT_INPUT2, szString, MAX_PATH);
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
                 { L"bin files", L"*.bin" },
                 { L"All Files", L"*.*" },
            };
            if (!CCFileSave(hDlg, szString, &pszFilename, FALSE, 2, rawType, L"*.bin")) {
                return (INT_PTR)TRUE;
            }
            {
                wcscpy_s(szString, pszFilename);
                CoTaskMemFree(pszFilename);
            }

            SetDlgItemText(hDlg, IDC_IMAGE_OUTPUT, szString);
            return (INT_PTR)TRUE;
        }

        case IDOK:
        {
            BOOL bSuccess;
            int NumSteps;
            BOOL UseIterations = FALSE;

            UseIterations = IsDlgButtonChecked(hDlg, IDC_USE_STEPS) == BST_CHECKED;
            if (UseIterations) {
                WritePrivateProfileString(L"SendASISdlg", L"UseIterations", L"1", (LPCTSTR)strAppNameINI);
            }
            else {
                WritePrivateProfileString(L"SendASISdlg", L"UseIterations", L"0", (LPCTSTR)strAppNameINI);
            }

            NumSteps = GetDlgItemInt(hDlg, IDC_NUM_BCA_STEPS, &bSuccess, TRUE);
            if (!bSuccess) {
                MessageBox(hDlg, L"Invalid # iterations", L"Bad Number", MB_OK);
                return (INT_PTR)TRUE;
            }
            if (NumSteps < 0) {
                MessageBox(hDlg, L"# iterations must be positive", L"Bad Number", MB_OK);
                return (INT_PTR)TRUE;
            }

            // check if this can be encoded as a unary expression in 79 bits 
            std::vector<std::vector<int>> combinations;

            if (UseIterations) {
                if (NumSteps != 0) {
                    combinations = factorCombinations(NumSteps);

                    // only need to look at first combination
                    int NumBits = 0;
                    auto& combination = combinations[0];
                    for (int factor : combination) {
                        NumBits += factor;
                    }

                    if (NumBits > 79) {
                        // Clear the combinations vector before reusing it
                        combinations.clear();
                        MessageBox(hDlg, L"# iterations can NOT be encoded into unary formatted footer\nChoose a different number of iterations",
                            L"Unary number exceeds 79 bits", MB_OK);
                        return (INT_PTR)TRUE;
                    }
                }
            }

            GetDlgItemText(hDlg, IDC_NUM_BCA_STEPS, szString, MAX_PATH);
            WritePrivateProfileString(L"SendASISdlg", L"BCAiterations", szString, (LPCTSTR)strAppNameINI);
            
            int UseThisThreshold;

            UseThisThreshold = GetDlgItemInt(hDlg, IDC_THRESHOLD, &bSuccess, TRUE);
            if (!bSuccess || UseThisThreshold <= 0) {
                if (UseIterations) {
                    combinations.clear();
                }
                MessageBox(hDlg, L"Invalid threshold or <= 0", L"Bad Number", MB_OK);
                return (INT_PTR)TRUE;
            }

            GetDlgItemText(hDlg, IDC_THRESHOLD, szString, MAX_PATH);
            WritePrivateProfileString(L"SendASISdlg", L"Threshold", szString, (LPCTSTR)strAppNameINI);

            GetDlgItemText(hDlg, IDC_IMAGE_INPUT, szString, MAX_PATH);
            WritePrivateProfileString(L"SendASISdlg", L"ImageInput", szString, (LPCTSTR)strAppNameINI);
            
            // read the image file
            int iRes;
            BYTE Header[10];
            BYTE Footer[10] = { 0,0,0,0,0,0,0,0,0,0 };
            int* InputImage = nullptr;
            IMAGINGHEADER ImageHeader;

            // Try .raw file format first

            iRes = LoadImageFile(&InputImage, szString, &ImageHeader);
            if (iRes != APP_SUCCESS) {
                // then try .bmp format
                iRes = ReadBMPfile(&InputImage, szString, &ImageHeader);
                if (iRes != APP_SUCCESS) {
                    if (UseIterations) {
                        combinations.clear();
                    }
                    MessageBox(hDlg, L"Input image file is not valid", L"File read error", MB_OK);
                    return (INT_PTR)TRUE;
                }
            }

            if (ImageHeader.Xsize != 256 || ImageHeader.Ysize != 256) {
                if (UseIterations) {
                    combinations.clear();
                }
                MessageBox(hDlg, L"Input image file must be 256Hx256V", L"Image format error", MB_OK);
                return (INT_PTR)TRUE;
            }

            // Read Header from text file
            GetDlgItemText(hDlg, IDC_TEXT_INPUT1, szString, MAX_PATH);
            iRes = ReadBYTEs2Text(szString, Header, 10, FALSE);
            if (iRes != APP_SUCCESS) {
                delete[]InputImage;
                if (UseIterations) {
                    combinations.clear();
                }
                MessageBox(hDlg, L"Header bit text file not valid", L"File read error", MB_OK);
                return (INT_PTR)TRUE;
            }
            WritePrivateProfileString(L"SendASISdlg", L"TextInput1", szString, (LPCTSTR)strAppNameINI);

            
            // Read Footer from text file
            GetDlgItemText(hDlg, IDC_TEXT_INPUT2, szString, MAX_PATH);
            if (!UseIterations) {
                iRes = ReadBYTEs2Text(szString, Footer, 10, FALSE);
                if (iRes != APP_SUCCESS) {
                    delete[]InputImage;
                    MessageBox(hDlg, L"Footer bit text file not valid", L"File read error", MB_OK);
                    return (INT_PTR)TRUE;
                }
                WritePrivateProfileString(L"SendASISdlg", L"TextInput2", szString, (LPCTSTR)strAppNameINI);
            }
            else {
                if (NumSteps != 0) {
                    // create footer from factors combination list
                    BYTE CurrentByte = 0;
                    int CurrentBit = 0;
                    BOOL OddBit = TRUE;
                    auto& combination = combinations[0];

                    for (int factor : combination) {
                        // set factor number of bits
                        for (int i = 0; i < factor; i++) {
                            if (OddBit) {
                                // set to ones
                                Footer[CurrentByte] = Footer[CurrentByte] | (0x80 >> CurrentBit);
                            }
                            // the footer is initillay already all zeros so no need to set those
                            CurrentBit++;
                            if (CurrentBit >= 8) {
                                CurrentBit = 0;
                                CurrentByte++;
                                if (CurrentByte >= 10) {
                                    // things are broken, this should never happens
                                    combinations.clear();
                                    delete[] InputImage;
                                    MessageBox(hDlg, L"Program failure generating footer\nfrom iterations specified",
                                        L"Array boundary error", MB_OK);
                                    return (INT_PTR)TRUE;
                                }
                            }
                        }
                        OddBit = !OddBit;
                    }
                    if (OddBit) {
                        // fill out rest of bits with 1s out to 80
                        if (CurrentBit != 0) {
                            for (int i = CurrentBit; i < 8; i++) {
                                Footer[CurrentByte] = Footer[CurrentByte] | (0x80 >> i);
                            }
                            CurrentByte++;
                        }
                        for (int i = CurrentByte; i < 10; i++) {
                            Footer[CurrentByte] = 0xff;
                        }
                    }
                }
                // no need to zero fill, already done at initialization
                // this also works for 0 iterations
            }
            
            if (UseIterations) {
                combinations.clear();
            }

            if (ImageHeader.NumFrames == 3) {
                CollapseImageFrames(InputImage, &ImageHeader, UseThisThreshold);
            }
            else {
                BinarizeImage(InputImage, &BCAimageHeader, UseThisThreshold);
            }

            if (NumSteps != 0) {
                // run BCA to encode
                // single point CCW rules for BCA
                int Rules[16] = { 0, 4, 1, 3, 8, 5, 6, 7, 2, 9,10,11,12,13,14,15 };
                int Histo[5] = { 0,0,0,0,0 };
                BOOL EvenStep = TRUE;
   
                for (int i = 0; i < NumSteps; i++) {
                    MargolusBCAp1p1(EvenStep, InputImage,
                        ImageHeader.Xsize, ImageHeader.Ysize, Rules, Histo);
                    EvenStep = !EvenStep;
                }
            }

            GetDlgItemText(hDlg, IDC_IMAGE_OUTPUT, szString, MAX_PATH);
            /*
            {
                // diagnostic section
                WCHAR BMPFilename[MAX_PATH];
                WCHAR RawFilename[MAX_PATH];
                WCHAR Drive[_MAX_DRIVE];
                WCHAR Dir[_MAX_DIR];
                WCHAR Fname[_MAX_FNAME];
                WCHAR Ext[_MAX_EXT];

                // split apart original filename
                int err;
                err = _wsplitpath_s(szString, Drive, _MAX_DRIVE, Dir, _MAX_DIR, Fname,
                    _MAX_FNAME, Ext, _MAX_EXT);
                if (err != 0) {
                    delete[]InputImage;
                    MessageBox(hDlg, L"Could not create diagnostic output filename", L"Send ASIS message", MB_OK);
                    return (INT_PTR)TRUE;
                }

                // reassemble filename as .raw file
                err = _wmakepath_s(RawFilename, _MAX_PATH, Drive, Dir, L"diagnostic", L".raw");
                if (err != 0) {
                    delete[]InputImage;
                    MessageBox(hDlg, L"Could not create diagnostic output filename", L"Send ASIS message", MB_OK);
                    return (INT_PTR)TRUE;
                }
                iRes = SaveImageFile(hDlg, InputImage, RawFilename, &ImageHeader);

                // reassemble filename as .bmp file
                err = _wmakepath_s(BMPFilename, _MAX_PATH, Drive, Dir, L"diagnostic", L".bmp");
                if (err != 0) {
                    delete[]InputImage;
                   MessageBox(hDlg, L"Could not create output filename", L"Send ASIS message", MB_OK);
                    return (INT_PTR)TRUE;
                }

                iRes = SaveBMP(BMPFilename, RawFilename, FALSE, TRUE);
                if (iRes == APP_SUCCESS) {
                    iRes = GetPrivateProfileInt(L"SettingsGlobalDlg", L"AutoPNG", 1, (LPCTSTR)strAppNameINI);
                    if (iRes != 0) {
                        SaveBMP2PNG(BMPFilename);
                    }
                }

            }
            */

            // Save Image to bitstream
            BYTE* MessageBody=nullptr;
            int BitCount;
            iRes = ConvertImage2Bitstream(InputImage, &ImageHeader, &MessageBody, 8192, &BitCount);
            if (iRes != APP_SUCCESS) {
                delete[] InputImage;
                return (INT_PTR)TRUE;
            }
            delete[] InputImage;

            iRes = SaveASISbitstream(szString, Header, MessageBody, Footer);
            if (iRes != APP_SUCCESS) {
                delete [] MessageBody;
                MessageBox(hDlg, L"Could not save ASIS bitstream", L"File output error", MB_OK);
                return (INT_PTR)TRUE;
            }

            delete[] MessageBody;

            WritePrivateProfileString(L"SendASISdlg", L"MessageOutput", szString, (LPCTSTR)strAppNameINI);

            {
                // save window position/size data
                CString csString = L"SendASISdlg";
                SaveWindowPlacement(hDlg, csString);
            }

            WCHAR NewMessage[MAX_PATH];
            swprintf_s(NewMessage, MAX_PATH, L"Generate message complete, %d bits set in image", BitCount);
            MessageBox(hDlg, NewMessage, L"Send ASIS message", MB_OK);

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        case IDCANCEL:
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        }
    }

    return (INT_PTR)FALSE;
}

//*******************************************************************************
//
// Message handler for UnaryDlg dialog box.
// 
//*******************************************************************************
INT_PTR CALLBACK UnaryDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        WCHAR szString[MAX_PATH];

    case WM_INITDIALOG:
    {
        int BCAiterations = 0;

        GetPrivateProfileString(L"UnaryDlg", L"BCAiterations",
            L"6625", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_NUM_BCA_STEPS, szString);

        GetPrivateProfileString(L"UnaryDlg", L"TextOutput1", L"C:\\MySETIBCA\\Data\\Results\\Unary.txt", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_TEXT_OUTPUT1, szString);

        int ResetWindows = GetPrivateProfileInt(L"GlobalSettings", L"ResetWindows", 0, (LPCTSTR)strAppNameINI);
        if (!ResetWindows) {
            CString csString = L"UnaryDlg";
            RestoreWindowPlacement(hDlg, csString);
        }

        GetPrivateProfileString(L"UnaryDlg", L"LastValue", L"12", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_LAST_VALUE, szString);

        GetPrivateProfileString(L"UnaryDlg", L"NumBits", L"80", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_NUM_BITS, szString);

        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDC_TEXT_OUTPUT1_BROWSE:
        {
            PWSTR pszFilename;
            GetDlgItemText(hDlg, IDC_TEXT_OUTPUT1, szString, MAX_PATH);
            COMDLG_FILTERSPEC textType[] =
            {
                 { L"text files", L"*.txt" },
                 { L"All Files", L"*.*" },
            };
            if (!CCFileSave(hDlg, szString, &pszFilename, FALSE, 2, textType, L"*.txt")) {
                return (INT_PTR)TRUE;
            }
            {
                wcscpy_s(szString, pszFilename);
                CoTaskMemFree(pszFilename);
            }
            SetDlgItemText(hDlg, IDC_TEXT_OUTPUT1, szString);
            return (INT_PTR)TRUE;
        }

        case IDOK:
        {
            BOOL bSuccess;
            int NumSteps;
            int LastValue;
            int NumBitsString;

            NumSteps = GetDlgItemInt(hDlg, IDC_NUM_BCA_STEPS, &bSuccess, TRUE);
            if (!bSuccess) {
                MessageBox(hDlg, L"Invalid # iterations", L"Bad Number", MB_OK);
                return (INT_PTR)TRUE;
            }
            if (NumSteps < 0) {
                MessageBox(hDlg, L"# iterations must be positive", L"Bad Number", MB_OK);
                return (INT_PTR)TRUE;
            }

            NumBitsString = GetDlgItemInt(hDlg, IDC_NUM_BITS, &bSuccess, TRUE);
            if (!bSuccess) {
                MessageBox(hDlg, L"Invalid # bit string length", L"Bad Number", MB_OK);
                return (INT_PTR)TRUE;
            }
            if (NumBitsString < 10 || NumBitsString > 160) {
                MessageBox(hDlg, L"Bit string length must be in range 10 to 160 ", L"Bad Number", MB_OK);
                return (INT_PTR)TRUE;
            }

            LastValue = GetDlgItemInt(hDlg, IDC_LAST_VALUE, &bSuccess, TRUE);
            if (!bSuccess) {
                MessageBox(hDlg, L"Invalid # Last Unary value", L"Bad Number", MB_OK);
                return (INT_PTR)TRUE;
            }
            if (LastValue < 1 || LastValue > (NumBitsString-1)) {
                MessageBox(hDlg, L"Last Value must be in range 1 to (Bit String Length - 1)", L"Bad Number", MB_OK);
                return (INT_PTR)TRUE;
            }

            GetDlgItemText(hDlg, IDC_NUM_BCA_STEPS, szString, MAX_PATH);
            WritePrivateProfileString(L"UnaryDlg", L"BCAiterations", szString, (LPCTSTR)strAppNameINI);

            GetDlgItemText(hDlg, IDC_LAST_VALUE, szString, MAX_PATH);
            WritePrivateProfileString(L"UnaryDlg", L"LastValue", szString, (LPCTSTR)strAppNameINI);

            GetDlgItemText(hDlg, IDC_NUM_BITS, szString, MAX_PATH);
            WritePrivateProfileString(L"UnaryDlg", L"NumBits", szString, (LPCTSTR)strAppNameINI);

            GetDlgItemText(hDlg, IDC_TEXT_OUTPUT1, szString, MAX_PATH);
            WritePrivateProfileString(L"UnaryDlg", L"TextOutput1", szString, (LPCTSTR)strAppNameINI);

            FILE* Out;
            errno_t ErrNum;

            ErrNum = _wfopen_s(&Out, szString, L"w");
            if (Out == NULL) {
                WCHAR NewMessage[MAX_PATH];
                swprintf_s(NewMessage, MAX_PATH, L"Cpuld not create file:\n%s", szString);
                MessageBox(hDlg, NewMessage, L"Unary Sequence message", MB_OK);
                return (INT_PTR)TRUE;
            }

            int SeqCount = 0;
            std::vector<std::vector<int>> combinations;

            for (int i = 2; i <= NumSteps; i++) {
                
                // check if this can be encoded as a unary expression in 79 bits
                combinations = factorCombinations(i);
                // only need to look at first combination

                int NumBits = 0;
                auto& combination = combinations[0];
                for (int factor : combination) {
                    NumBits += factor;
                }
                if (NumBits == (NumBitsString-LastValue)) {
                    SeqCount++;
                    fprintf(Out, "%d: %d = ", SeqCount, i);
                    int j = 0;
                    for (int factor : combination) {
                        if (j == 0) {
                            fprintf(Out, "%d", factor);
                        }
                        else {
                            fprintf(Out, " * %d", factor);
                        }
                        NumBits += factor;
                        j++;
                    }
                    fprintf(Out, ", %d\n", LastValue);
                }

                combinations.clear();
            }

            fclose(Out);

            {
                // save window position/size data
                CString csString = L"UnaryDlg";
                SaveWindowPlacement(hDlg, csString);
            }

            WCHAR NewMessage[MAX_PATH];
            swprintf_s(NewMessage, MAX_PATH, L"Unary sequences complete, %d valid sequences", SeqCount);
            MessageBox(hDlg, NewMessage, L"Unary Sequence message", MB_OK);

            return (INT_PTR)TRUE;
        }

        case IDCANCEL:
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        }
    }

    return (INT_PTR)FALSE;
}

//*******************************************************************************
//
// Message handler for GenericFSMdlg dialog box.
// 
//*******************************************************************************
INT_PTR CALLBACK GenericFSMdlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
        WCHAR szString[MAX_PATH];

    case WM_INITDIALOG:
    {
        if (MyFSM == NULL) {
            MyFSM = new GenericFSM;
            if (MyFSM == nullptr) {
                MessageBox(hDlg, L"Could not create GenericFSM class", L"Fatal Error", MB_OK);
                EndDialog(hDlg, LOWORD(wParam));
                return (INT_PTR)TRUE;
            }
        }

        GetPrivateProfileString(L"GenericFSMdlg", L"InputSeq", L"n1-b-n3-n2-n3-a-n3-n2-n2-b-n3-n2-b-b-n1-n3", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_INPUT, szString);

        GetPrivateProfileString(L"GenericFSMdlg", L"FSMini", L"C:\MySETIBCA\Data\Amino Acids\ASIS Codon\Jason1.ini", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
        SetDlgItemText(hDlg, IDC_FSM_INI, szString);

        int ResetWindows = GetPrivateProfileInt(L"GlobalSettings", L"ResetWindows", 0, (LPCTSTR)strAppNameINI);
        if (!ResetWindows) {
            CString csString = L"GenericFSMdlg";
            RestoreWindowPlacement(hDlg, csString);
        }

        SetDlgItemText(hDlg, IDC_RESULTS, L"");

        HWND ItemHandle = GetDlgItem(hDlg, IDC_RUN);
        EnableWindow(ItemHandle, FALSE);

        ItemHandle = GetDlgItem(hDlg, IDC_RESET);
        EnableWindow(ItemHandle, FALSE);

        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDC_FSM_INI_BROWSE:
        {
            PWSTR pszFilename;
            GetDlgItemText(hDlg, IDC_FSM_INI, szString, MAX_PATH);
            COMDLG_FILTERSPEC textType[] =
            {
                 { L"text files", L"*.ini" },
                 { L"All Files", L"*.*" },
            };
            if (!CCFileOpen(hDlg, szString, &pszFilename, FALSE, 2, textType, L"*.ini")) {
                return (INT_PTR)TRUE;
            }
            {
                wcscpy_s(szString, pszFilename);
                CoTaskMemFree(pszFilename);
            }

            SetDlgItemText(hDlg, IDC_FSM_INI, szString);

            return (INT_PTR)TRUE;
        }

        case IDC_RUN:
        {
            // only enabled after FSM is loaded
            WCHAR Sequence[MAX_SYMBOL_SIZE*4];
            GetDlgItemText(hDlg, IDC_INPUT, Sequence, MAX_SYMBOL_SIZE * 4);

            ResetTheFSM(hDlg,TRUE); // reset FSM and clear the results

            ProcessSequenceUsingFSM(hDlg, Sequence, MAX_SYMBOL_SIZE * 4);

            ResetTheFSM(hDlg, FALSE); // reset FSM and leave the results

            return (INT_PTR)TRUE;
        }

        case IDC_LOAD:
        {
            GetDlgItemText(hDlg, IDC_FSM_INI, szString, MAX_PATH);
            WritePrivateProfileString(L"GenericFSMdlg", L"FSMini", szString, (LPCTSTR)strAppNameINI);

            int Warnings;
            int iRet;

            iRet = MyFSM->LoadFSM(szString, &Warnings);

            if (iRet != APP_SUCCESS) {
                if ((Warnings & ERROR_FSM_NO_START_STATE) != 0) {
                    MessageBox(hDlg, L"FSM is missing the start=state\nin the [states] section", L"Generic FSM", MB_OK);
                }

                if ((Warnings & ERROR_FSM_NO_INPUTS) != 0) {
                    MessageBox(hDlg, L"FSM is missing [inputs] section", L"Generic FSM", MB_OK);
                }

                if ((Warnings & ERROR_FSM_NO_OUTPUTS) != 0) {
                    MessageBox(hDlg, L"FSM is missing the [outputs] section", L"Generic FSM", MB_OK);
                }

                if ((Warnings & ERROR_FSM_NO_STATES) != 0) {
                    MessageBox(hDlg, L"FSM is missing the [states] section", L"Generic FSM", MB_OK);
                }

                if ((Warnings & ERROR_FSM_NO_STATE_RULES) != 0) {
                    MessageBox(hDlg, L"FSM is missing the [next_state_rules] section", L"Generic FSM", MB_OK);
                }

                if ((Warnings & ERROR_FSM_NO_OUTPUT_RULES) != 0) {
                    MessageBox(hDlg, L"FSM is missing the [output_rules] section", L"Generic FSM", MB_OK);
                }

                MessageBox(hDlg, L"Failed to load FSM, check ini file", L"Generic FSM", MB_OK);
                return (INT_PTR)TRUE;
            }

            if(Warnings != 0) {
                MessageBox(hDlg, L"Warnings, FSM incomplete, check ini file", L"Generic FSM", MB_OK);
            }

            //
            // load input alphabet listbox
            // 
            HWND hwndList = GetDlgItem(hDlg, IDC_INPUT_ALPHABET);
            // clear listbox contents
            SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
            int NumInputs = MyFSM->GetNumInputAlphabet();
            for (int i = 0; i < NumInputs; i++)
            {
                WCHAR Label[MAX_SYMBOL_SIZE];
                int index;

                MyFSM->GetInputAlphabetLabel(i, Label, MAX_SYMBOL_SIZE);

                index = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)Label);
                // Set the array index of the StateLabel as item data.
                // This enables us to retrieve the item from the array
                // even after the items are sorted by the list box.
                SendMessage(hwndList, LB_SETITEMDATA, index, (LPARAM)i);
            }

            //
            // load output alphabet listbox
            //
            hwndList = GetDlgItem(hDlg, IDC_OUTPUT_ALPHABET);
            // clear listbox contents
            SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
            int NumOutputs = MyFSM->GetNumOutputAlpabet();
            for (int i = 0; i < NumOutputs; i++)
            {
                WCHAR Label[MAX_SYMBOL_SIZE];
                int index;

                MyFSM->GetOutputAlphabetLabel(i, Label, MAX_SYMBOL_SIZE);
                if (MyFSM->IsOutputUsed(i)) {
                    index = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)Label);
                } else {
                    WCHAR Bad[MAX_SYMBOL_SIZE * 2];
                    swprintf_s(Bad, MAX_SYMBOL_SIZE * 2, L"%d=%s, not used", i, Label);
                    index = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)Bad);
                }
                // Set the array index of the StateLabel as item data.
                // This enables us to retrieve the item from the array
                // even after the items are sorted by the list box.
                SendMessage(hwndList, LB_SETITEMDATA, index, (LPARAM)i);
            }

            // load state symbol listbox
            // IDC_STATES_LIST
            hwndList = GetDlgItem(hDlg, IDC_STATES_LIST);
            // clear listbox contents
            SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
            int NumStates = MyFSM->GetNumStates();
            for (int i = 0; i < NumStates; i++)
            {
                WCHAR Label[MAX_SYMBOL_SIZE];
                int index;

                MyFSM->GetStateLabel(i, Label, MAX_SYMBOL_SIZE);
                if (MyFSM->IsStateUsed(i)) {
                    index = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)Label);
                }
                else {
                    WCHAR Bad[MAX_SYMBOL_SIZE * 2];
                    swprintf_s(Bad, MAX_SYMBOL_SIZE * 2, L"%d=%s,Unreachable", i, Label);
                    index = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)Bad);
                }
                // Set the array index of the StateLabel as item data.
                // This enables us to retrieve the item from the array
                // even after the items are sorted by the list box.
                SendMessage(hwndList, LB_SETITEMDATA, index, (LPARAM)i);
            }

            // load state/output rules listbox
            // IDC_RULES_LIST
            
            hwndList = GetDlgItem(hDlg, IDC_RULES_LIST);
            // clear listbox contents
            SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
            for (int i = 0; i < NumStates; i++) {
                for(int j=0; j<NumInputs; j++) {
                    WCHAR Label[MAX_SYMBOL_SIZE*5];
                    WCHAR OutputSymbol[MAX_SYMBOL_SIZE];
                    WCHAR NextStateSymbol[MAX_SYMBOL_SIZE];
                    WCHAR StateSymbol[MAX_SYMBOL_SIZE];
                    WCHAR InputSymbol[MAX_SYMBOL_SIZE];
                    WCHAR Unreachable[MAX_SYMBOL_SIZE];

                    int index;
                    int NextState;
                    int Output;
                   
                    if (!MyFSM->IsStateUsed(i)) {
                        wcscpy_s(Unreachable, MAX_SYMBOL_SIZE, L"Unreachable: ");
                    } else {
                        wcscpy_s(Unreachable, MAX_SYMBOL_SIZE, L"");
                    }
                    NextState = MyFSM->GetNextStateRule(i, j);
                    Output = MyFSM->GetOuputRule(i, j);
                    MyFSM->GetOutputAlphabetLabel(Output, OutputSymbol, MAX_SYMBOL_SIZE);
                    MyFSM->GetStateLabel(NextState, NextStateSymbol, MAX_SYMBOL_SIZE);
                    MyFSM->GetInputAlphabetLabel(j, InputSymbol, MAX_SYMBOL_SIZE);
                    MyFSM->GetStateLabel(i, StateSymbol, MAX_SYMBOL_SIZE);

                    // 4 possibilities
                    if (NextState >= 0 && Output >= 0) {
                        // valid entry for both next state and output
                        swprintf_s(Label, MAX_SYMBOL_SIZE * 5, L"%s%d.%d:%s.%s -> (%d:%s) out=%s", Unreachable, i, j, StateSymbol, InputSymbol, NextState, NextStateSymbol, OutputSymbol);
                    } else if (NextState >= 0 && Output < 0) {
                        // valid entry for next state, invalid entry for output
                        swprintf_s(Label, MAX_SYMBOL_SIZE * 5, L"%s%d.%d:%s.%s -> (%d:%s) out=%s", Unreachable, i, j, StateSymbol, InputSymbol, NextState, NextStateSymbol, L"*invalid*");
                    }
                    else if (NextState < 0 && Output >= 0) {
                        // next state invalid and output is valid
                        swprintf_s(Label, MAX_SYMBOL_SIZE * 5, L"%s%d.%d:%s.%s -> (%d:%s) out=%s", Unreachable, i, j, StateSymbol, InputSymbol, NextState, L"*invalid*", OutputSymbol);
                    } else {
                        // both next state and output are invalid
                        swprintf_s(Label, MAX_SYMBOL_SIZE * 5, L"%s%d.%d:%s.%s -> (%d:%s) out=%s", Unreachable, i, j, StateSymbol, InputSymbol, NextState, L"*invalid*", L"*invalid*");
                    }

                    index = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)Label);
                    // Set the array index of the StateLabel as item data.
                    // This enables us to retrieve the item from the array
                    // even after the items are sorted by the list box.
                    SendMessage(hwndList, LB_SETITEMDATA, index, (LPARAM)i);
                }
            }
            
            // set current state status box
            // IDC_CURRENT_STATE
            {
                int index;
                index = MyFSM->GetCurrentStateIndex();
                if (index >= 0) {
                    WCHAR Symbol[MAX_SYMBOL_SIZE];
                    MyFSM->GetStateLabel(index, Symbol, MAX_SYMBOL_SIZE);
                    SetDlgItemText(hDlg, IDC_CURRENT_STATE, Symbol);
                } else {
                    SetDlgItemText(hDlg, IDC_CURRENT_STATE, L"<invalid>");
                }
            }

            // set current output status box
            // IDC_CURRENT_OUTPUT
            {
                int iRet;
                WCHAR Symbol[MAX_SYMBOL_SIZE];
                iRet = MyFSM->GetCurrentOutput(Symbol, MAX_SYMBOL_SIZE);
                if (iRet==APP_SUCCESS) {
                    SetDlgItemText(hDlg, IDC_CURRENT_OUTPUT, Symbol);
                } else {
                    SetDlgItemText(hDlg, IDC_CURRENT_OUTPUT, L"<invalid>");
                }
            }

            HWND ItemHandle = GetDlgItem(hDlg, IDC_RUN);
            EnableWindow(ItemHandle, TRUE);

            ItemHandle = GetDlgItem(hDlg, IDC_RESET);
            EnableWindow(ItemHandle, TRUE);

            return (INT_PTR)TRUE;
        }

        case IDC_RESET:
        {
            ResetTheFSM(hDlg, TRUE);

            return (INT_PTR)TRUE;
        }

        case IDC_INPUT_ALPHABET:
        {
            // Step the FSM with the input selected by a double click on the input alphabet

            if (HIWORD(wParam) == LBN_DBLCLK) {
                HWND ItemHandle = GetDlgItem(hDlg, IDC_INPUT_ALPHABET);
                int index = (int)SendMessage(ItemHandle, LB_GETCURSEL, 0, 0);
                MyFSM->StepFSM(index);

                // update the current state text box
                index = MyFSM->GetCurrentStateIndex();
                if (index >= 0) {
                    WCHAR Symbol[MAX_SYMBOL_SIZE];
                    MyFSM->GetStateLabel(index, Symbol, MAX_SYMBOL_SIZE);
                    SetDlgItemText(hDlg, IDC_CURRENT_STATE, Symbol);
                }
                else {
                    SetDlgItemText(hDlg, IDC_CURRENT_STATE, L"<invalid>");
                }

                // update the current output text box
                // and the results text box

                WCHAR Symbol[MAX_SYMBOL_SIZE];
                WCHAR Results[MAX_SYMBOL_SIZE * 2];
                GetDlgItemText(hDlg, IDC_RESULTS, Results, MAX_SYMBOL_SIZE * 2);

                int iRet = MyFSM->GetCurrentOutput(Symbol, MAX_SYMBOL_SIZE);
                if (iRet == APP_SUCCESS) {
                    // Append the current output to the contents of the results text box
                    if (wcscmp(Symbol, L"<space>") == 0) {
                        wcscat_s(Results, MAX_SYMBOL_SIZE * 2, L" ");
                    }
                    else if (wcscmp(Symbol, L"<no>") != 0) {
                        wcscat_s(Results, MAX_SYMBOL_SIZE * 2, Symbol);
                    } // if this was <no> do not generate output
                }
                else {
                    // Append the current output to the contents of the results text box
                    wcscat_s(Results, MAX_SYMBOL_SIZE * 2, L"<invalid>");
                }

                SetDlgItemText(hDlg, IDC_RESULTS, Results);

            }
            return(INT_PTR)TRUE;
        }

        case IDOK:
        {
            GetDlgItemText(hDlg, IDC_INPUT, szString, MAX_PATH);
            WritePrivateProfileString(L"GenericFSMdlg", L"InputSeq", szString, (LPCTSTR)strAppNameINI);

            GetDlgItemText(hDlg, IDC_FSM_INI, szString, MAX_PATH);
            WritePrivateProfileString(L"GenericFSMdlg", L"FSMini", szString, (LPCTSTR)strAppNameINI);

            {
                // save window position/size data
                CString csString = L"GenericFSMdlg";
                SaveWindowPlacement(hDlg, csString);
            }

            // delete FSM class
            if (MyFSM != nullptr) {
                delete MyFSM;
                MyFSM = nullptr;
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        case IDCANCEL:
        {
            // delete FSM class
            if (MyFSM != nullptr) {
                delete MyFSM;
                MyFSM = nullptr;
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }

        }
    }

    return (INT_PTR)FALSE;
}

//*******************************************************************************
//
// Helper function for GenericFSMdlg dialog box.
// 
//*******************************************************************************
void ResetTheFSM(HWND hDlg,BOOL ClearResults)
{
    MyFSM->ResetFSM();
    // set current state status box
    WCHAR Symbol[MAX_SYMBOL_SIZE];
    int State = MyFSM->GetCurrentStateIndex();
    if (State < 0) {
        SetDlgItemText(hDlg, IDC_CURRENT_STATE, L"<invalid>");
    }
    else {
        MyFSM->GetStateLabel(State, Symbol, MAX_SYMBOL_SIZE);
        SetDlgItemText(hDlg, IDC_CURRENT_STATE, Symbol);
    }

    // set current output status box
    MyFSM->GetCurrentOutput(Symbol, MAX_SYMBOL_SIZE);
    SetDlgItemText(hDlg, IDC_CURRENT_OUTPUT, Symbol);

    // clear the results box
    if (ClearResults) {
        SetDlgItemText(hDlg, IDC_RESULTS, L"");
    }

    return;
}

//*******************************************************************************
//
// Helper function for GenericFSMdlg dialog box.
// 
//*******************************************************************************
int ProcessSequenceUsingFSM(HWND hDlg, WCHAR* Sequence, size_t MaxSeqLength)
{
    // use comma separated input string
    // process each input until none are left
    // save results to the Results string text box
    // token separators can be ,<space>,-

    WCHAR* Token = NULL;
    WCHAR* NextToken = NULL;
    WCHAR Delimiters[MAX_SYMBOL_SIZE];

    int iRet = MyFSM->GetDelimiters(Delimiters, MAX_SYMBOL_SIZE);
    if (iRet != APP_SUCCESS) {
        MessageBox(hDlg, L"Missing delimters definition\nCheck FSM ini file", L"Run FSM", MB_OK);
        return APPERR_PARAMETER;
    }

    Token = wcstok_s(Sequence, Delimiters, &NextToken);
    while (Token != NULL) {
        iRet = MyFSM->StepFSM(Token);

        if (iRet != APP_SUCCESS) {
            WCHAR Message[MAX_SYMBOL_SIZE];
            swprintf_s(Message, MAX_SYMBOL_SIZE, L"Bad input symbol: %s\nnot found in input alphabet\nterminating run", Token);
            MessageBox(hDlg,Message,L"Run FSM",MB_OK);
            return APPERR_PARAMETER;
        }
        WCHAR Symbol[MAX_SYMBOL_SIZE];
        WCHAR Results[MAX_SYMBOL_SIZE * 2];
        GetDlgItemText(hDlg, IDC_RESULTS, Results, MAX_SYMBOL_SIZE * 2);

        iRet = MyFSM->GetCurrentOutput(Symbol, MAX_SYMBOL_SIZE);
        if (iRet == APP_SUCCESS) {
            // Append the current output to the contents of the results text box
            if (wcscmp(Symbol, L"<space>") == 0) {
                wcscat_s(Results, MAX_SYMBOL_SIZE * 2, L" ");
            } else if (wcscmp(Symbol, L"<no>") != 0) {
                wcscat_s(Results, MAX_SYMBOL_SIZE * 2, Symbol);
            } // if this was <no> do not generate output
        }
        else {
            // Append the current output to the contents of the results text box
            wcscat_s(Results, MAX_SYMBOL_SIZE * 2, L"<invalid>");
        }

        SetDlgItemText(hDlg, IDC_RESULTS, Results);

        Token = wcstok_s(NULL, Delimiters, &NextToken);
    }

    return APP_SUCCESS;
}