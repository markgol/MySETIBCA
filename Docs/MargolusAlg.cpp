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
