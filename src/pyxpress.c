/* http://www.pyflag.net/pyflag/src/lib/pyxpress.c
 *
 *
 *** A c implementation of xpress decode for speed...
 *
 *
     This code is taken verbatim from Sandman and modified for forensic integrity.

     SandMan framework.
     Copyright 2008 (c) Matthieu Suiche. <msuiche[at]gmail.com>
     Updated by Simson Garfinkel, 2011.

     This file was originally part of SandMan.

     SandMan is free software: you can redistribute it and/or modify
     it under the terms of the GNU General Public License as published by
     the Free Software Foundation, either version 3 of the License, or
     (at your option) any later version.

     SandMan is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with SandMan.  If not, see <http://www.gnu.org/licenses/>.

     * In addition, as a special exception, the copyright holders give
     * permission to link the code of portions of this program with the
     * OpenSSL library under certain conditions as described in each
     * individual source file, and distribute linked combinations
     * including the two.
     *
     * You must obey the GNU General Public License in all respects
     * for all of the code used other than OpenSSL.  If you modify
     * file(s) with this exception, you may extend this exception to your
     * version of the file(s), but you are not obligated to do so.  If you
     * do not wish to do so, delete this exception statement from your
     * version.  If you delete this exception statement from all source
     * files in the program, then also delete it here.

     Module Name:

     Abstract:

     - Windows hibernation functions implementation.
     - BEWARE: Very ugly code. Sorry.

     Environment:

     - User mode

     Revision History:

     - 04-08-2008 Final version.
     - 06-23-2008 New implementation of Xpress encode and decode.(msuiche)
     - Matthieu Suiche

     --*/

/*++
  Function Name: Xpress_Decompress

  Overview:

  Parameters:
  -

  Return Values:
  -
  --*/

#include "config.h"
#include "pyxpress.h"
#include <stdint.h>

/* This one is arleady defined on Windows */

typedef unsigned char * PUCHAR;
typedef unsigned char UCHAR;
typedef uint16_t USHORT;

// 32-bit systems
#define PAGE_SIZE               0x00001000
#define XPRESS_MAGIC   "\x81\x81xpress"
#define XPRESS_ENCODE_MAGIC  0x19880922
#define DELTA_PAGE ((2 * PAGE_SIZE) - 1)
#define UNCOMPRESSED_BLOCK_SIZE (PAGE_SIZE * 0x10)

unsigned long Xpress_Decompress(const unsigned char *InputBuffer,
				unsigned long InputSize,
				unsigned char *OutputBuffer,
				unsigned long OutputSize)
{
    uint32_t OutputIndex=0;
    uint32_t InputIndex=0;
    uint32_t Indicator=0;
    uint32_t IndicatorBit=0;
    uint32_t Length=0;
    uint32_t Offset=0;
    uint32_t NibbleIndex=0;
    //uint32_t NibbleIndicator = XPRESS_ENCODE_MAGIC;

    while ((OutputIndex < OutputSize) && (InputIndex<InputSize) ) {
        if (IndicatorBit == 0) {
	    if(InputIndex+3 >= InputSize) return OutputIndex;
            Indicator = (unsigned)((InputBuffer[InputIndex + 3] << 24)
				   | (InputBuffer[InputIndex + 2] << 16)
				   | (InputBuffer[InputIndex + 1] <<  8)
				   | InputBuffer[InputIndex]);
            InputIndex += sizeof(uint32_t);
            IndicatorBit = 32; 
        }
        IndicatorBit--;

	//* check whether the bit specified by IndicatorBit is set or not 
	//* set in Indicator. For example, if IndicatorBit has value 4 
	//* check whether the 4th bit of the value in Indicator is set 

        if (((Indicator >> IndicatorBit) & 1) == 0) {
	    if(OutputIndex>=OutputSize) return OutputIndex;
            OutputBuffer[OutputIndex] = InputBuffer[InputIndex]; 
            InputIndex += sizeof(UCHAR);
            OutputIndex += sizeof(UCHAR);
        }
        else {
	    if(InputIndex+1 >= InputSize) return OutputIndex;
            Length = (unsigned)((InputBuffer[InputIndex + 1] << 8) | InputBuffer[InputIndex]);
            
            /*
	      if ((OutputIndex > 0xD0) && (OutputIndex < 0xF0))
	      {
	      printf("DECOMP: READ AT [0x%08X] = %04X \n", InputIndex, Length);
	      }
            */

            InputIndex += sizeof(USHORT); 
            Offset = Length / 8;
            Length = Length % 8;

            //if ((OutputIndex > 0xD0) && (OutputIndex < 0xF0)) printf("--1 Len: %02X (%d)\n", Length, Length);
            if (Length == 7) {
                if (NibbleIndex == 0) {
                    NibbleIndex = InputIndex;
		    if(InputIndex>=InputSize) return OutputIndex;
                    Length = InputBuffer[InputIndex] % 16; 
                    //if ((OutputIndex > 0xD0) && (OutputIndex < 0xF0)) printf("--2 Len: %02X (%d)\n", Length, Length);
                    InputIndex += sizeof(UCHAR);
                }
                else {
                    Length = InputBuffer[NibbleIndex] / 16;
                    //if ((OutputIndex > 0xD0) && (OutputIndex < 0xF0)) printf("--3 Len: %02X (%d)\n", Length, Length);
                    NibbleIndex = 0;
                }

                if (Length == 15) {
		    if (InputIndex>=InputSize) return OutputIndex;
                    Length = InputBuffer[InputIndex];
                    //if ((OutputIndex > 0xD0) && (OutputIndex < 0xF0)) printf("--4 Len: %02X (%d)\n", Length, Length);
                    InputIndex += sizeof(UCHAR); 
		    if (Length == 255) {
			if(InputIndex+2>=InputSize) return OutputIndex;
			Length = (unsigned)((InputBuffer[InputIndex + 1] << 8)) | InputBuffer[InputIndex];
			InputIndex += sizeof(USHORT);
			Length -= (15 + 7); 
		    }
                    Length += 15; 
                    //if ((OutputIndex > 0xD0) && (OutputIndex < 0xF0)) printf("--5 Len: %02X (%d)\n", Length, Length);
                }
                Length += 7;
                //if ((OutputIndex > 0xD0) && (OutputIndex < 0xF0)) printf("--6 Len: %02X (%d)\n", Length, Length);
            }

            Length += 3;
            //if ((OutputIndex > 0xD0) && (OutputIndex < 0xF0)) printf("--7 Len: %02X (%d)\n", Length, Length);
            //if (Length > 280) printf("DECOMP DEBUG: [0x%08X]->[0x%08X] Len: %d Offset: %08X\n", 
            //    OutputIndex, InputIndex, Length, Offset);
            while (Length != 0) {
                if ((OutputIndex >= OutputSize) || ((Offset + 1) >= OutputIndex)) break;
                OutputBuffer[OutputIndex] = OutputBuffer[OutputIndex - Offset - 1];
                OutputIndex += sizeof(UCHAR);
                Length -= sizeof(UCHAR);
            }
        }
    }
    return OutputIndex;
}

#ifdef FOR_PYTHON
#include <Python.h>

static PyObject *xpress_decode(PyObject *self, PyObject *args) {
    char *inbuff;
    Py_ssize_t insize;

    char *outbuff;
    Py_ssize_t outsize;
    PyObject *result;

    if(!PyArg_ParseTuple(args, "s#", &inbuff, &insize))
	return NULL;

    outsize = UNCOMPRESSED_BLOCK_SIZE;
    result = PyString_FromStringAndSize(NULL, outsize);
    if(!result) return NULL;

    outbuff = PyString_AsString(result);
    outsize = Xpress_Decompress(inbuff, outbuff, outsize);

    // Truncate buffer back to outsize:
    if(_PyString_Resize(&result, outsize) < 0) 
	return NULL;

    return result;
};

static PyMethodDef pyxpressMethods[] = {
    {"decode", (PyCFunction)xpress_decode, METH_VARARGS,
     "decode a buffer" },
    {NULL, NULL, 0, NULL}
};

static PyObject *g_module_reference;

PyMODINIT_FUNC initpyxpress(void) {
    Py_InitModule("pyxpress", pyxpressMethods);
}
#endif
