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
//     -7 File write error
//
// Some function return TRUE/FALSE results
//
// V1.0.0	2024-06-21	Initial release
// V1.1.0   2024-06-28  Added ASIS receive/send message
//                          Recieve reads the ASIS .bin bitstream file and saves a decoded
//                          .raw image file and a .bmp file
//                          Send will encode a .raw image file or .bmp file that is 256x256
//                          and save it as a ASIS bitstream file.
//                          functions added to support this:
//                              ReadASISmessage()
//                              BitSequences()
//                              ConvertImage2Bitstream()
//                              CollapseImageFrames() (moved here from CADialogs.cpp)
//                              BinarizeImage()
// V1.1.1   2024-07-01  Corrected bug that locked rule file with program was running
// V1.1.2   2024-07-08  Added CountBitInImage()
// V1.1.3   2024-07-18  Correction, allow 0 iterations for ASIS message
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
#include <cmath>
#include <vector>
#include "AppErrors.h"
#include "ImageDialog.h"
#include "Globals.h"
#include "imageheader.h"
#include "FileFunctions.h"
#include "CA.h"

// These are the state globals that start, stop and track processing

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
//  int* Histo                  count of 0,1,2,3,4 #pixel set in 2x2 bloock
// 
//  return value:
//  1 - Success
//  !=1 Error see standardized app error list at top of this source file
//
//*******************************************************************************
void MargolusBCAp1p1(BOOL EvenStep, int* TheImage, int Xsize, int Ysize,
    int* Rules, int* Histo)
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
            Histo[0]++;
            break;

        case 1:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 0;
            TheImage[(yp1 * Xsize) + x] = 0;
            TheImage[(yp1 * Xsize) + xp1] = 0;
            Histo[1]++;
            break;

        case 2:
            TheImage[(y * Xsize) + x] = 0;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 0;
            TheImage[(yp1 * Xsize) + xp1] = 0;
            Histo[1]++;
            break;

        case 3:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 0;
            TheImage[(yp1 * Xsize) + xp1] = 0;
            Histo[2]++;
            break;

        case 4:
            TheImage[(y * Xsize) + x] = 0;
            TheImage[(y * Xsize) + xp1] = 0;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 0;
            Histo[1]++;
            break;

        case 5:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 0;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 0;
            Histo[2]++;
            break;

        case 6:
            TheImage[(y * Xsize) + x] = 0;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 0;
            Histo[2]++;
            break;

        case 7:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 0;
            Histo[3]++;
            break;

        case 8:
            TheImage[(y * Xsize) + x] = 0;
            TheImage[(y * Xsize) + xp1] = 0;
            TheImage[(yp1 * Xsize) + x] = 0;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            Histo[1]++;
            break;

        case 9:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 0;
            TheImage[(yp1 * Xsize) + x] = 0;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            Histo[2]++;
            break;

        case 10:
            TheImage[(y * Xsize) + x] = 0;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 0;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            Histo[2]++;
            break;

        case 11:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 0;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            Histo[3]++;
            break;

        case 12:
            TheImage[(y * Xsize) + x] = 0;
            TheImage[(y * Xsize) + xp1] = 0;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            Histo[2]++;
            break;

        case 13:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 0;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            Histo[3]++;
            break;

        case 14:
            TheImage[(y * Xsize) + x] = 0;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            Histo[3]++; 
            break;

        case 15:
            TheImage[(y * Xsize) + x] = 255;
            TheImage[(y * Xsize) + xp1] = 255;
            TheImage[(yp1 * Xsize) + x] = 255;
            TheImage[(yp1 * Xsize) + xp1] = 255;
            Histo[4]++;
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
    fclose(TextIn);
    return APP_SUCCESS;
}

//******************************************************************************
//
//  
// 
//******************************************************************************
int ReadASISmessage(WCHAR *Filename, IMAGINGHEADER* ImageHeader, int** NewImage,
    BYTE* Header, BYTE* Footer, int* BCAiterations, int* BitCount)
{
    FILE* In;
    size_t iRead;

    //
    // open Filename
    _wfopen_s(&In, Filename, L"rb");
    if (In == NULL) {
        *NewImage = NULL;
        return APPERR_FILEOPEN;
    }

    // read first 10 bytes into Header
    // verify header format
    iRead = fread(Header, 1, 10, In);
    if (iRead != 10) {
        *NewImage = NULL;
        fclose(In);
        return APPERR_FILEREAD;
    }
    if (Header[0] != 0xff || Header[1] != 0xff) {
        // This is not an ASIS message
        *NewImage = NULL;
        fclose(In);
        return APPERR_PARAMETER;
    }
    if (Header[2] != 0x06 || Header[3] != 0x90) {
        // This may be unknown ASIS message type
        *NewImage = NULL;
        fclose(In);
        return APPERR_PARAMETER;
    }

    if (Header[6] != 0x44 || Header[7] != 0x88) {
        // This may be unknown ASIS message type
        *NewImage = NULL;
        fclose(In);
        return APPERR_PARAMETER;
    }
    if (Header[8] != 0x44 || Header[9] != 0x88) {
        // This may be unknown ASIS message type
        *NewImage = NULL;
        fclose(In);
        return APPERR_PARAMETER;
    }

    // Allocate 8192 byte array
    BYTE* MessageBody;
    MessageBody = new BYTE[(size_t)8192];  // alocate array of byte to receive image
    if (MessageBody == nullptr) {
        *NewImage = NULL;
        fclose(In);
        return APPERR_MEMALLOC;
    }
    // read 8192 bytes from file
    iRead = fread(MessageBody, 1, 8192, In);
    if (iRead != 8192) {
        // not ASIS message
        delete[] MessageBody;
        *NewImage = NULL;
        fclose(In);
        return APPERR_FILEREAD;
    }

    // read last 10 bytes into footer
    iRead = fread(Footer, 1, 10, In);
    if (iRead != 10) {
        // not ASIS message
        delete[] MessageBody;
        *NewImage = NULL;
        fclose(In);
        return APPERR_FILEREAD;
    }
    BYTE Tmp;
    iRead = fread(&Tmp, 1, 1, In);
    if (iRead == 1) {
        // not ASIS message
        delete[] MessageBody;
        *NewImage = NULL;
        fclose(In);
        return APPERR_PARAMETER;
    }
    fclose(In);
    
    // count the bit sequences in footer (both 1s and 0s)
    int BitCountList[40];
    int NumSequences;
    NumSequences = BitSequences(Footer, BitCountList, 10, FALSE);
    if (NumSequences == 0) {
        delete[] MessageBody;
        *NewImage = NULL;
        return APPERR_PARAMETER;
    }

    if (NumSequences <= 1 ) {
        // do not run the BCA
        *BCAiterations = 0;
    }
    else {
        // mulitply the length all sequences but the last
        // use this as the BCAiterations required
        *BCAiterations = 1;
        for (int i = 0; i < (NumSequences - 1); i++) {
            *BCAiterations *= BitCountList[i];
        }
    }

    // Allocae the new image array
    int* Image;
    Image = new int[(size_t)65536];
    if (Image == nullptr) {
        delete[] MessageBody;
        *NewImage = NULL;
        return APPERR_MEMALLOC;
    }

    // convert 8192 message into 65536 entries the NewImage
    int Count=0;
    for (int i = 0,k=0; i < 8192; i++) {
        for (int j = 0; j < 8; j++) {
            if ((MessageBody[i] & ((BYTE)0x80 >> j)) == 0) {
                Image[k] = 0;
            }
            else {
                Count++;
                Image[k] = 255;
            }
            k++;
        }
    }
    delete[] MessageBody;

    // buld ImageHeader
    ImageHeader->Endian = (short)-1;  // PC format
    ImageHeader->HeaderSize = (short)sizeof(IMAGINGHEADER);
    ImageHeader->ID = (short)0xaaaa;
    ImageHeader->Version = (short)1;
    ImageHeader->NumFrames = (short)1;
    ImageHeader->PixelSize = 1;
    ImageHeader->Xsize = 256;
    ImageHeader->Ysize = 256;
    ImageHeader->Padding[0] = 0;
    ImageHeader->Padding[1] = 0;
    ImageHeader->Padding[2] = 0;
    ImageHeader->Padding[3] = 0;
    ImageHeader->Padding[4] = 0;
    ImageHeader->Padding[5] = 0;

    *BitCount = Count;
    *NewImage = Image;
    return APP_SUCCESS;
}

//*******************************************************************
//
//  BitSequences
// 
// Generates a table of each bit value
// of 1 sequence and the # of 1 bits in the sequence
// 
// Parameters:
// HWND hDlg            Handle of calling window/dialog
// WCHAR* InputFile     Packed binary bitstream file
// WCHAR* OutputFile    CSV text output file
// int SkipSize         # of bits to skip before starting (typically the prologue)
//
//*******************************************************************
int BitSequences(BYTE *ByteList, int* BitCountList, int MaxBytes, BOOL BitOrder)
{
    int CurrentBit = 0;
    int CurrentPrologueBit = 0;
    int CurrentByteBit = 0;
    int NumOnes = 0;
    int NumZeros = 0;
    int LastBit = 0;
    int BitCounter = 0;
    int OneLength = 0;
    int ZeroLength = 0;
    int NumSequences = 0;
    int CurrentState = 0; // 0 - this is first bit in file, 1 - counting 1 sequence, 2 counting 0 sequence

    for(int i=0; i< MaxBytes; i++) {
        char CurrentByte;
        int BitValue;

        CurrentByteBit = 0;

        CurrentByte = ByteList[i];

        // process data bit by bit in the order of the bit transmission message
        // input file is byte oriented MSB to LSB representing the bit order that the message was received
        // This does not imply any bit ordering in the message itself.

        while (CurrentByteBit < 8) {
            // current bit
            if (BitOrder == 0) {
                BitValue = CurrentByte & (0x80 >> CurrentByteBit);
            }
            else {
                BitValue = CurrentByte & (0x01 << CurrentByteBit);
            }
            // process this bit
            // possible states:
            //      first bit
            //      counting zeros
            //      counting ones
            // 
            switch (CurrentState) {
            case 0:
            {
                if (BitValue) {
                    CurrentState = 1;
                    NumOnes = 1;
                    OneLength = 1;
                }
                else {
                    CurrentState = 2;
                    NumZeros = 1;
                    ZeroLength = 1;
                }
                break;
            }

            case 1:
            {
                if (BitValue) {
                    // another 1
                    NumOnes++;
                    OneLength++;
                    break;
                }

                // this is a 0, so change states
                // save result of 1 sequence
                BitCountList[NumSequences] = OneLength;
                NumSequences++;
                ZeroLength = 1;
                NumZeros++;
                CurrentState = 2;
                break;
            }

            case 2:
            {
                if (BitValue == 0) {
                    // another 0
                    NumZeros++;
                    ZeroLength++;
                    break;
                }

                // this is a 1, so change states
                // save result of 0 sequence
                BitCountList[NumSequences] = ZeroLength;
                NumSequences++;
                OneLength = 1;
                NumOnes++;
                CurrentState = 1;
                break;
            }

            default:
                // should never get here
                break;
            };

            CurrentByteBit++;
            CurrentBit++;
        }
    }

    // save last state results
    if (CurrentState == 1) {
        BitCountList[NumSequences] = OneLength;
        NumSequences++;
    }
    else if (CurrentState == 2) {
        BitCountList[NumSequences] = ZeroLength;
        NumSequences++;
    }

    return NumSequences;
}

//*******************************************************************
//
//  ConvertImage2Bitstream
//
//*******************************************************************
int ConvertImage2Bitstream(int* InputImage, IMAGINGHEADER* ImageHeader,
    BYTE **MessageBody, int MessageLength, int* BitCount)
{
    if ((ImageHeader->Xsize * ImageHeader->Ysize / 8) != MessageLength) {
        *MessageBody = nullptr;
        return APPERR_PARAMETER;
    }

    BYTE* Message;
    Message = new BYTE[MessageLength];
    if (Message == nullptr) {
        *MessageBody = nullptr;
        return APPERR_MEMALLOC;
    }

    // convert
    BYTE CurrentByte;
    int ImageIndex = 0;
    int Count = 0;

    for (int i = 0; i < MessageLength; i++) {
        CurrentByte = 0;
        for (int j = 0; j < 8; j++) {
            if (InputImage[ImageIndex] != 0) {
                    Count++;
                    CurrentByte = CurrentByte | (0x80 >> j);
            }
            ImageIndex++;
        }
        Message[i] = CurrentByte;
    }

    *MessageBody = Message;
    *BitCount = Count;
    return APP_SUCCESS;
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
void CollapseImageFrames(int* Image, IMAGINGHEADER* ImageHeader, int Threshold) {
    int FrameOffset;
    int x, y;
    int Offset;

    FrameOffset = ImageHeader->Xsize * ImageHeader->Ysize;

    for (y = 0, Offset = 0; y < ImageHeader->Ysize; y++) {
        for (x = 0; x < ImageHeader->Xsize; x++) {
            Image[Offset + x] = Image[Offset + x] + Image[FrameOffset + Offset + x] +
                Image[2 * FrameOffset + Offset + x];
            if (Image[Offset + x] < Threshold) {
                Image[Offset + x] = 0;
            }
            else {
                Image[Offset + x] = 255;
            }
        }
        Offset += ImageHeader->Xsize;
    }

    ImageHeader->NumFrames = 1;
    return;
}

//*******************************************************************************
//
// BinarizeImage
// 
// This is to binarize image from single frame
// Use Threshold to binarize
// 
// int* TheImage
// IMAGINGHEADER* BCAimageHeader
// 
// no return parameter
// 
//*******************************************************************************
void BinarizeImage(int* TheImage, IMAGINGHEADER* BCAimageHeader, int Threshold)
{
    for (int i = 0; i < (BCAimageHeader->Xsize * BCAimageHeader->Ysize*BCAimageHeader->NumFrames); i++) {
        if (TheImage[i] < Threshold) {
            TheImage[i] = 0;
        }
        else {
            TheImage[i] = 255;
        }
    }
    return;
}

//*******************************************************************************
//
// BinarizeImage
// 
// This is to binarize image from single frame
// Use Threshold to binarize
// 
// int* TheImage
// IMAGINGHEADER* BCAimageHeader
// 
// return parameter
// 
//  -1  Error can not count bits in image reasons could be:
//          There is no image
//          # frames > 1
// 
//  0 to Xsize*Ysize, # of non zero pixels
//          
//*******************************************************************************
int CountBitInImage(int* Image, IMAGINGHEADER* ImageHeader)
{
    int Count = 0;
    if (Image==nullptr || ImageHeader->NumFrames > 1) {
        return -1;
    }
    for (int i = 0; i < (ImageHeader->Xsize * ImageHeader->Ysize * ImageHeader->NumFrames); i++) {
        if (Image[i] > 0) {
            Count++;
        }
    }

    return Count;
}

//***************************************************************
//
// These functions were generated by chatgpt
// 
// Function to find all combinations of factors that multiply to a given number
//      findFactors()
// Function to generate all factor combinations
//      factorCombinations()
//***************************************************************
// Function to find all combinations of factors that multiply to a given number
void findFactors(int num, int start, std::vector<int>& current, std::vector<std::vector<int>>& result) {
    // Base case: if num becomes 1, we have found a valid combination
    if (num == 1) {
        if (!current.empty()) {
            result.push_back(current);
        }
        return;
    }

    // Start from the last factor added to avoid duplicates
    for (int i = start; i <= num; ++i) {
        if (num % i == 0) {
            current.push_back(i);
            findFactors(num / i, i, current, result);
            current.pop_back(); // backtrack
        }
    }
}

// Function to generate all factor combinations
std::vector<std::vector<int>> factorCombinations(int num) {
    std::vector<std::vector<int>> result;
    std::vector<int> current;
    findFactors(num, 2, current, result); // start with 2 to avoid including 1 in the combinations
    return result;
}


// kikuchiyo (Discord username)
// used zlib and the compression size as a entropy measure

