//
// MySETIBCA, an application for decoding, encoding message images using 
// a block cellular automata like what was used in the 'A Sign inSpace' project message
// 
// CA.cpp
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
// V1.0.0	2024-06-21	Initial release
//
//  This contains the Margolus block cellular functions
//  This will get converted to a c++ class
//  
//  Still to do:
//      Implement the forward BCA algorithim (even step)
//      Implement the backward BCA algorithim (odd step)
//      Implement continuous forward  as a background process
//      Implement continuous backward as a background process
//
#include "framework.h"
#include <stdio.h>
#include <wchar.h>
#include <atlstr.h>
#include <strsafe.h>
#include "AppErrors.h"
#include "ImageDialog.h"
#include "Globals.h"
#include "imageheader.h"
#include "FileFunctions.h"
#include "CA.h"

// THese are the state globals that start, stop and track processing

// BCArunning == 0  && StopRunning == TRUE
//  no processing is running
// BCSrunning == -1 && StopRunnig = TRUE
//  set BCSrunning to 0 (stopped) at the end the current iteration
// BCSrunning == 1 && StopRunnig = TRUE
//  set BCSrunning to 0 (stopped) at the end the current iteration
// BCSrunning == -1 and StopRunning = FALSE
//  run backward iteration
// BCSrunning == 0 and StopRunning = FALSE
//  run foward iteration

int BCArunning = 0; // -1 running backward
                    // 0 stopped
                    // +1 runing forward

BOOL BCAimageLoaded = FALSE;

int CurrentIteration = 0; // also used to tell whether current frame is even or odd
BOOL EvenStep = TRUE;      // Even step in Margolus neighborhood 
BOOL StopRunning = TRUE; // request to stop iterations at the end of the  current iteration
                         // This is set to FALSE to start runnning iterations
                         // StateBCA will indicate backwards or forwards
int ForwardRules[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };
int BackwardRules[16] = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

int* TheImage;
IMAGINGHEADER BCAimageHeader;

//******************************************************************************
//
// 2x2 block number assignment (i.e. wwhich bits are set in the 2x2 block)
// 
// 2x2
// 00  10  01  11  00  10  01  11  00  10  01  11  00  10  01  11
// 00  00  00  00  10  10  10  10  01  01  01  01  11  11  11  11
// 
// Number
//  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
// 
//******************************************************************************


//******************************************************************************
//
// MargolusBCAp1p1
// 
// This implements a step for a 2x2 Margolus block cellular automata cell
// 
//  BOOL EvenStep               Identifies a even or odd interation (step)
//  int* TheImage               Pointer to the image
//  int Xsize                   y size of image
//  int Ysize                   y size of image
//  int* Rules                  list of the 16 block substituion rules
// 
//  return value:
//  1 - Success
//  !=1 Error see standardized app error list at top of this source file
//
//*******************************************************************************
void MargolusBCAp1p1(BOOL EvenStep, int* TheImage, int Xsize, int Ysize,
    int* Rules)
{
    int Length = Xsize * Ysize / 4; // number of 2x2 blocks in image
    int Cell = 0;
    int i;
    int x, y;
    int xp1, yp1;
    int StartX, StartY;

    if (EvenStep) {
        // 2x2 grid starts at 0,0
        StartX = 0;
        StartY = 0;
    }
    else {
        // 2x2 grid starts at 1,1
        StartX = 1;
        StartY = 1;
    }

    // convert TheImage into 2x2 blocks
    for (i = 0, x = StartX, y = StartY; i < Length; i++) {
        // calculate value of a 2x2 block
        // convert the 2x2 block into a number 0-15
        //******************************************************************************
        //
        // 2x2 block number assignment (i.e. which bits are set in the 2x2 block)
        // 
        // 2x2
        // 00  10  01  11  00  10  01  11  00  10  01  11  00  10  01  11
        // 00  00  00  00  10  10  10  10  01  01  01  01  11  11  11  11
        // 
        // Number
        //  0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15
        // 
        //******************************************************************************
        Cell = 0;
        // convert the 2x2 block to a 0-15 number
        // use the modulo to handle wrap around space on the boundaries as required

        xp1 = (x + 1) % Xsize;
        yp1 = (y + 1) % Ysize;

        // UL cell 
        if (TheImage[(y * Xsize) + x] != 0) {
            Cell = 1;
        }
        // UR cell
        if (TheImage[(y * Xsize) + xp1] != 0) {
            Cell = Cell + 2;
        }
        // LL cell
        if (TheImage[(yp1 * Xsize) + x] != 0) {
            Cell = Cell + 4;
        }
        // LR cell
        if (TheImage[(yp1 * Xsize) + xp1] != 0) {
            Cell = Cell + 8;
        }

        // Convert the 2x2 using the Rules
        Cell = Rules[Cell];
        switch (Cell) {
        case 0:
            TheImage[(y * Xsize) + x] = 0;
            TheImage[(y * Xsize) + xp1] = 0;
            TheImage[(yp1 * Xsize) + x] = 0;
            TheImage[(yp1 * Xsize) + xp1] = 0;
            break;

        case 1:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 0;
            TheImage[(yp1 * Xsize) + x] = 0;
            TheImage[(yp1 * Xsize) + xp1] = 0;
            break;

        case 2:
            TheImage[(y * Xsize) + x] = 0;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 0;
            TheImage[(yp1 * Xsize) + xp1] = 0;
            break;

        case 3:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 0;
            TheImage[(yp1 * Xsize) + xp1] = 0;
            break;

        case 4:
            TheImage[(y * Xsize) + x] = 0;
            TheImage[(y * Xsize) + xp1] = 0;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 0;
            break;

        case 5:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 0;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 0;
            break;

        case 6:
            TheImage[(y * Xsize) + x] = 0;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 0;
            break;

        case 7:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 0;
            break;

        case 8:
            TheImage[(y * Xsize) + x] = 0;
            TheImage[(y * Xsize) + xp1] = 0;
            TheImage[(yp1 * Xsize) + x] = 0;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            break;

        case 9:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 0;
            TheImage[(yp1 * Xsize) + x] = 0;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            break;

        case 10:
            TheImage[(y * Xsize) + x] = 0;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 0;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            break;

        case 11:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 0;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            break;

        case 12:
            TheImage[(y * Xsize) + x] = 0;
            TheImage[(y * Xsize) + xp1] = 0;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            break;

        case 13:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 0;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            break;

        case 14:
            TheImage[(y * Xsize) + x] = 0;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            break;

        case 15:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            break;
        }

        x = x + 2;
        if (x >= Xsize) {
            x = StartX;
            y = y + 2;
        }
    }

    return;
}

//******************************************************************************
//
// ReadFulesFile
// 
// Parameters:
//	HWND hDlg				Handle of calling window or dialog
//	WCHAR* InputFile		Input binary file
//	int*   Rules            List of transition rules, contains 16 rules
// 
//  return value:
//  1 - Success
//  !=1 Error see standardized app error list at top of this source file
//
//*******************************************************************************
int ReadRulesFile(HWND hDlg, WCHAR* InputFile, int* Rules)
{
    // read in rules file
    int iRes;
    errno_t ErrNum;
    FILE* TextIn;

    ErrNum = _wfopen_s(&TextIn, InputFile, L"r");
    if (!TextIn) {
        return APPERR_FILEOPEN;
    }

    iRes = fscanf_s(TextIn, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
        &Rules[0], &Rules[1], &Rules[2], &Rules[3],
        &Rules[4], &Rules[5], &Rules[6], &Rules[7],
        &Rules[8], &Rules[9], &Rules[10], &Rules[11],
        &Rules[12], &Rules[13], &Rules[14], &Rules[15]);

    if (iRes != 16) {
        fclose(TextIn);
        return APPERR_FILEREAD;
    }

    for (int i = 0; i < 16; i++) {
        if (Rules[i] < 0 || Rules[i]>15) {
            return APPERR_PARAMETER;
        }
    }

    return APP_SUCCESS;
}

// kikuchiyo (Discord username)
// used zlib and the compression size as a entropy measure

