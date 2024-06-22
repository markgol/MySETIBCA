//
// MySETIBCA, an application for decoding, encoding message images using 
// a block cellular automata like what was used in the 'A Sign inSpace' project message
// 
// ImageDlg.cpp
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
// If not, see < https://www.gnu.org/licenses/>. x
// 
//
// V1.0.0	2024-06-21	Initial release
// 
//  This module is a copy of the ImageDlg module used in MySETIviewer and customized
//  for this application
//
// This file contains the dialog callback procedures for the image dialog
// 
// The Status bar based on example from:
//      https://learn.microsoft.com/en-us/windows/win32/controls/create-status-bars
// 
// This handles all the actual display of the bitmap generated
//
#include "framework.h"
#include "resource.h"
#include <shobjidl.h>
#include <winver.h>
#include <windowsx.h>
#include <vector>
#include <atlstr.h>
#include <strsafe.h>
#include <shellapi.h>
#include <d2d1.h>
#include <d2d1_1.h>
#pragma comment(lib, "d2d1.lib")
#include "globals.h"
#include "AppFunctions.h"
#include "ImageDialog.h"

extern void ApplyDisplay(HWND hDlg);

//*******************************************************************************
//
// Message handler for ImageDlg dialog box.
// This window is used to display an image or BMP file
// It is a modeless dialog.
// 
//*******************************************************************************
INT_PTR CALLBACK ImageDlg(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    //    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {

    case WM_INITDIALOG:
    {
        ImgDlg->InitializeDirect2D();

        int ResetWindows = GetPrivateProfileInt(L"GlobalSettings", L"ResetWindows", 0, (LPCTSTR)strAppNameINI);
        if (!ResetWindows) {
            CString csString = L"ImageWindow";
            RestoreWindowPlacement(hDlg, csString);
        
            WCHAR szString[MAX_PATH];
            int iRes;
            float sf, pox, poy;

            // restore scaleFactor
            GetPrivateProfileString(L"ImageDialog", L"scaleFactor", L"1.0",szString, MAX_PATH, (LPCTSTR)strAppNameINI);
            iRes = swscanf_s(szString, L"%f", &sf);
            if (iRes != 1) {
                sf = 1.0;
            }
            ImgDlg->SetScale(sf);

            // restore panOffset
            GetPrivateProfileString(L"ImageDialog", L"panOffsetX", L"0.0", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
            iRes = swscanf_s(szString, L"%f", &pox);
            if (iRes != 1) {
                pox = 0.0;
            }
            GetPrivateProfileString(L"ImageDialog", L"panOffsetY", L"0.0", szString, MAX_PATH, (LPCTSTR)strAppNameINI);
            iRes = swscanf_s(szString, L"%f", &poy);
            if (iRes != 1) {
                poy = 0.0;
            }
            ImgDlg->SetPan(pox, poy);
        }

        ImgDlg->CreateStatusBar(hDlg, ID_IMG_STATUSBAR, hInst);
        if (ShowStatusBar) {
            ImgDlg->ShowStatusBar(TRUE);
        }
        else {
            ImgDlg->ShowStatusBar(FALSE);
        }
        return (INT_PTR)TRUE;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDCANCEL:
        case IDOK:
            return (INT_PTR)TRUE;

        case IDC_EXIT:
        {
            // save window position/size data
            CString csString = L"ImageWindow";
            SaveWindowPlacement(hDlg, csString);
            ShowWindow(hwndImage, SW_HIDE);
            return (INT_PTR)TRUE;
        }

        case IDM_RESET_ZOOM:
        {
            if (ImgDlg) {
                ImgDlg->SetScale(1.0f);
            }
            ImgDlg->Repaint();
            ImgDlg->UpdateStatusBar(hDlg);
            return (INT_PTR)TRUE;
        }

        case IDM_RESET_PAN:
        {
            if (ImgDlg) {
                ImgDlg->SetPan(0.0f, 0.0f);
            }
            ImgDlg->Repaint();
            ImgDlg->UpdateStatusBar(hDlg);
            return (INT_PTR)TRUE;
        }

        case IDC_GENERATE_BMP:
        {
            int xsize, ysize;
            COLORREF* Image;
            Displays->GetDisplay(&Image, &xsize, &ysize);
            
            if(ImgDlg->LoadCOLORREFimage(hwndImage, xsize, ysize,Image)) {
                ImgDlg->Repaint();
                ImgDlg->UpdateStatusBar(hDlg);
            }
            return (INT_PTR)TRUE;
        }

        default:
            return (INT_PTR)FALSE;
        } // end of WM_COMMAND

    case WM_SYSCOMMAND:
    {
        if (wParam == SC_CLOSE) {
            // save window position/size data
            CString csString = L"ImageWindow";
            SaveWindowPlacement(hDlg, csString);
            ShowWindow(hwndImage, SW_HIDE);
            return (INT_PTR)TRUE;
        }
        return (INT_PTR)FALSE;
    }

    case WM_WINDOWPOSCHANGING:
    {
        WINDOWPOS* wpos = (WINDOWPOS*)lParam;
        int x, y;

        if (!Displays || !Displays->GetSize(&x, &y)) {
            // don't show window if there is nothing to show
            wpos->flags &= ~SWP_SHOWWINDOW;
        }
        return 0;
    }

    case WM_MOUSEWHEEL:
    {
        ImgDlg->Rescale(GET_WHEEL_DELTA_WPARAM(wParam));
        ImgDlg->Repaint();
        ImgDlg->UpdateStatusBar(hDlg);
        break;
    }

    case WM_MOUSEMOVE:
    {
        int x, y;
        x = GET_X_LPARAM(lParam);
        y = GET_Y_LPARAM(lParam);

        if (ImgDlg->PanImage(hDlg,x,y)) {
            ImgDlg->Repaint();
        }

        ImgDlg->UpdateMousePos(hDlg, x, y);
        ImgDlg->UpdateStatusBar(hDlg);
        break;
    }

    case WM_LBUTTONDOWN:
        ImgDlg->EnablePanning(TRUE);
        SetCapture(hDlg); //ResizeStatusBar Capture the mouse input to receive WM_MOUSEMOVE even if the mouse is outside the window
        break;

    case WM_LBUTTONUP:
        ImgDlg->EnablePanning(FALSE);
        ReleaseCapture(); // Release the mouse input capture
        break;

    case WM_SIZE:
    {
        int xsize, ysize;
        COLORREF* Image;
        Displays->GetDisplay(&Image, &xsize, &ysize);

        if (ImgDlg->LoadCOLORREFimage(hwndImage, xsize, ysize, Image)) {
            ImgDlg->Repaint();
            ImgDlg->UpdateStatusBar(hDlg);
        }

        ImgDlg->ResizeStatusBar(hDlg);
        ImgDlg->UpdateStatusBar(hDlg);
        break;
    }

    case WM_DESTROY:
    {
        float sf, pox, poy;
        int iRes;
        WCHAR szString[40];

        // save scaleFactor
        sf = ImgDlg->GetScale();
        swprintf_s(szString, 40, L"%f", sf);
        WritePrivateProfileString(L"ImageDialog", L"scaleFactor", szString, (LPCTSTR)strAppNameINI);

        // save panOffset
        ImgDlg->GetPan(&pox, &poy);
        swprintf_s(szString, 40, L"%f", pox);
        WritePrivateProfileString(L"ImageDialog", L"panOffsetX", szString, (LPCTSTR)strAppNameINI);
        
        swprintf_s(szString, 40, L"%f", poy);
        WritePrivateProfileString(L"ImageDialog", L"panOffsetY", szString, (LPCTSTR)strAppNameINI);

        ImgDlg->DestroyStatusBar();

        // release Direct2D
        ImgDlg->ReleaseDirect2D();

        hwndImage = NULL;
        break;
    }

    }
    return (INT_PTR)FALSE;
}

