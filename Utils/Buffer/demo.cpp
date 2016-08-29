// Demo the data buffer area
#include "Buffer.h"
#include "BufferRow.h"
#include "BufferCol.h"
#include "BufferMat.h"

#include <iostream>

using std::cout;
using std::endl;

int main(int argc, char *args[])
{
  Buffer<char> b(4,5,2,2);
  b.clearData(0xD);
  b.clearWindow(0xB);

  cout << "Buffer is 4x5 with a 2x2 window." << endl;

  Buffer<char>::Row br(b, 2, true);
  br.v=2;
  br++;
  cout << "Row(2)=2" << endl << b << endl;
  br.v=3;
  cout << "Row(3)=3" << endl << b << endl;
  br.setRow(1);
  br.v=1;
  br.setRow();
  cout << "Row(1)=1" << endl << b << endl;
  br.v=0;
  br.flush();
  cout << "Row(0)=0" << endl << b << endl;
  br.v=3; // leaves the buffer in an inconsistent state until after the print.
  cout << "Row(0)=3 - no flush" << endl << b << endl;

  Buffer<char>::Column bc(b, 0, true);
  bc.v+=1;
  bc.setCol(2);
  cout << "Col(0)+=1" << endl << b << endl;
  bc.v=7;
  bc.flush();
  cout << "Col(2)=7" << endl << b << endl;

  Buffer<char>::Matrix bm0(b);
  bm0.m=0;
  bm0.flush();
  cout << "Reset to 0" << endl << b << endl;

  Buffer<char>::Matrix bm(b, 3, 3);
  bm.m=3;
  bm.flush();
  cout << "Top-left 3x3 = 3" << endl << b << endl;

  bm.setStart(2,2);
  bm.m=2;
  bm.setStart(1,1);
  cout << "The 3x3 block at (2,2) = 2" << endl << b << endl;

  bm.m=1;
  bm.flush();
  cout << "The 3x3 block at (1,1) = 1" << endl << b << endl;

}
