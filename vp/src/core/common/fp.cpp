#include "fp.h"

extern "C" {
#include <softfloat/internals.h>
#include <softfloat/specialize.h>
}


// These f32/64_classify functions are from the official Spike simulator (the softfloat library part):
/*****************************************************************************************************
Copyright (c) 2010-2017, The Regents of the University of California
(Regents).  All Rights Reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. Neither the name of the Regents nor the
   names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.

IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
HEREUNDER IS PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
*****************************************************************************************************/

uint_fast16_t f32_classify( float32_t a )
{
    union ui32_f32 uA;
    uint_fast32_t uiA;

    uA.f = a;
    uiA = uA.ui;

    uint_fast16_t infOrNaN = expF32UI( uiA ) == 0xFF;
    uint_fast16_t subnormalOrZero = expF32UI( uiA ) == 0;
    bool sign = signF32UI( uiA );
    bool fracZero = fracF32UI( uiA ) == 0;
    bool isNaN = isNaNF32UI( uiA );
    bool isSNaN = softfloat_isSigNaNF32UI( uiA );

    return
            (  sign && infOrNaN && fracZero )          << 0 |
            (  sign && !infOrNaN && !subnormalOrZero ) << 1 |
            (  sign && subnormalOrZero && !fracZero )  << 2 |
            (  sign && subnormalOrZero && fracZero )   << 3 |
            ( !sign && infOrNaN && fracZero )          << 7 |
            ( !sign && !infOrNaN && !subnormalOrZero ) << 6 |
            ( !sign && subnormalOrZero && !fracZero )  << 5 |
            ( !sign && subnormalOrZero && fracZero )   << 4 |
            ( isNaN &&  isSNaN )                       << 8 |
            ( isNaN && !isSNaN )                       << 9;
}


uint_fast16_t f64_classify( float64_t a )
{
    union ui64_f64 uA;
    uint_fast64_t uiA;

    uA.f = a;
    uiA = uA.ui;

    uint_fast16_t infOrNaN = expF64UI( uiA ) == 0x7FF;
    uint_fast16_t subnormalOrZero = expF64UI( uiA ) == 0;
    bool sign = signF64UI( uiA );
    bool fracZero = fracF64UI( uiA ) == 0;
    bool isNaN = isNaNF64UI( uiA );
    bool isSNaN = softfloat_isSigNaNF64UI( uiA );

    return
            (  sign && infOrNaN && fracZero )          << 0 |
            (  sign && !infOrNaN && !subnormalOrZero ) << 1 |
            (  sign && subnormalOrZero && !fracZero )  << 2 |
            (  sign && subnormalOrZero && fracZero )   << 3 |
            ( !sign && infOrNaN && fracZero )          << 7 |
            ( !sign && !infOrNaN && !subnormalOrZero ) << 6 |
            ( !sign && subnormalOrZero && !fracZero )  << 5 |
            ( !sign && subnormalOrZero && fracZero )   << 4 |
            ( isNaN &&  isSNaN )                       << 8 |
            ( isNaN && !isSNaN )                       << 9;
}