/***********************************************************************************

Inspried by PHPPLD.
2021 Rue Mohr.
This generic ROM-generator framework was customized for:

7 segment clock, HH:MM

INPUTS
 A0 A1 A2                              3 bitcounter input for serializer
 
 A3 A4 A5 A6 A7 A8 A9 A10 A11 A12:     10 bit minute counter

OUTPUTS

D2: Hours  1,10's     7 segment via 595 to BCD decoders
D3: Minute 1,10's     7 segment via 595 to BCD decoders



 
FEEDBACK
 D0 : Next address 0-7   via 595
 D1 : Next address 8-15  via 595



Address bits      8 bit rom size

       -- no parallel roms available --
     8                  2 k
     9                  4 k
     10                 8 k
     
       -- eeproms available from here --
     11                 16 k  (28C16)
     12                 32 k  (28C32)
     
       -- eprom practical sizes from here --
     13                 64 k  (2764)
     14                 128 k (27128)
     15                 256 k 
     16                 512 k
     17                 1 M  (27010)
     18                 2 M
     19                 4 M
     20                 8 M

       -- flash from here up --



**************************************************************************************/


#include <stdio.h>
#include <stdint.h>
#include "ROMLib.h"


// the number of address lines you need !!!???!!!
#define InputBits 13

// the output data size, 8 or 16
#define OutputBits 8

// default output value
#define DFOutput  0x00



// Tuck this one away!. Bit reverser!  Please dont use this in real fft code,
//   YOU KNOW how many bits your working with, and you can use a 
//   specific case generator for it.
uint8_t uniReverse(uint8_t i, uint8_t bits) {

  uint8_t r, m, b;
  r = 0;             // result
  m = 1 << (bits-1); // mask will travel right
  b = 1;             // bit will travel left
  
  while(m) {
    if (i&b) r |=m;
    b <<= 1;
    m >>= 1;  
  }
  
  return r;  

}


// count set bits, unrolled edition.
// if using assember shift into the carry and use addc, 0
uint8_t bitCount(uint16_t n) {  
   uint8_t rv;
   rv = 0;
   if (n & 0x0001) rv++;
   if (n & 0x0002) rv++;
   if (n & 0x0004) rv++;
   if (n & 0x0008) rv++;
   if (n & 0x0010) rv++;
   if (n & 0x0020) rv++;
   if (n & 0x0040) rv++;
   if (n & 0x0080) rv++;   
   if (n & 0x0100) rv++;
   if (n & 0x0200) rv++;
   if (n & 0x0400) rv++;
   if (n & 0x0800) rv++;
   if (n & 0x1000) rv++;
   if (n & 0x2000) rv++;
   if (n & 0x4000) rv++;
   if (n & 0x8000) rv++; 
   
   return rv;
}

// convert a character and bit position to a serial level, 10 bits per character. 
uint8_t SerialChar(char c, uint16_t bit) {
  
  switch (bit) {
    case 0: return 0; // start bit
    case 1: return (c & 0x01)!=0;
    case 2: return (c & 0x02)!=0;
    case 3: return (c & 0x04)!=0;
    case 4: return (c & 0x08)!=0;
    case 5: return (c & 0x10)!=0;
    case 6: return (c & 0x20)!=0;
    case 7: return (c & 0x40)!=0;
    case 8: return (c & 0x80)!=0;
    case 9: return 1; // stop bit          
  }

}

// convert a minute index into an hour/minute time
void iToTime (uint16_t i, uint8_t * hour10 , uint8_t * hour1, uint8_t * minute10, uint8_t * minute1  ) {
  
  for ((*hour10)    = 0; i > 599; i-=600, (*hour10)++);
  for ((*hour1)     = 0; i > 59;  i-=60,  (*hour1)++ );
  for ((*minute10)  = 0; i > 9;  i-=10,  (*minute10)++ );
  *minute1 = i;
  
  (*hour1)++;
  if ((*hour1) == 10){ (*hour1) = 0; (*hour10)++; }

}





int main(void) {

  uint16_t bitI;
  uint16_t countI;
  
  uint8_t  count1O, count2O, hr10O, hr1O, min10O, min1O; 
  uint8_t  hrO, minO;
  
  uint32_t AP;

  uint32_t out;  // leave it alone!

  setup();       // open output file.
  
  
  // loop thru each address
  for( A=0; A<(1<<InputBits); A++) { // A is a bitfield (a mash-up of the vars we want)
       
     //AP = (A+4)&0x1FFF; // offset everything 4 bits to invert A2
     AP = A;  
     // ---------------------- reset vars -------------------------
     
     // -------------------- build input values --------------------
    spliceValueFromField( &bitI,               AP,  3,   0, 1, 2);  
    spliceValueFromField( &countI,             AP,  10,  3, 4, 5, 6, 7, 8, 9, 10, 11, 12);  
   
   
    // ------------------------ do stuff ---------------------------
           
    //the +480)%720 offsets the clock to start at 9:00   
    iToTime ( (countI+480)%720, &hr10O , &hr1O, &min10O, &min1O ); // convert index to 12 hr time by minute
      
    countI++;
    if (countI > 719) countI = 0;  
    
  // printf("%d, %d %d:%d %d\n", countI, hr10O, hr1O, min10O, min1O);
    
    hrO     = hr1O  | (hr10O << 4);  // pack the hours codes 
    minO    = min1O | (min10O << 4); // pack the minutes codes
    
    printf( "outputs: %02X, %02X\n", hrO, minO);
            	    
    hrO   = ((0x80>>bitI) & hrO)?1:0;   // serialize    
    minO  = ((0x80>>bitI) & minO)?1:0;  // serialize
	 	    	        
    count1O = countI & 0xFF; // break counter down into 2 bytes
    count2O = countI >> 8; 
    
    count1O = ((0x80>>bitI) & count1O)?1:0; // serialize
    count2O = ((0x80>>bitI) & count2O)?1:0; // serialize    
    
                  
     // ------------------- reconstitute the output ------------------------------
     // assign default values for outputs     
     out = DFOutput;
          
     spliceFieldFromValue( &out, count1O,          1,  0); 
     spliceFieldFromValue( &out, count2O,          1,  1);
     
     spliceFieldFromValue( &out, minO,             1,  2);
     spliceFieldFromValue( &out, hrO,              1,  3);

               
     // submit entry to file
     write(fd, &out, OutputBits>>3);  // >>3 converts to bytes, leave it!
  }
  
  cleanup(); // close file
  
  return 0;
}









