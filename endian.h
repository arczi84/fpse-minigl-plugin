#ifndef _FPSE_ENDIAN_H_
#define _FPSE_ENDIAN_H_

#ifdef MSB_FIRST
#define C_SWAP16(val) ((((val) & 0xFF) << 8) | (((val) >> 8) & 0xFF))
#define C_SWAP32(val) ((((val) & 0xFF) << 24) | (((val) & 0xFF00) << 8) | (((val) >> 8) & 0xff00) | (((val) >> 24) & 0xFF))
#else
#define C_SWAP16(val) (val)
#define C_SWAP32(val) (val)
#endif

#ifdef __PPC__

#define SWAP16(val) ({ UINT16 x = val; x = (x << 8) | (x >> 8); x; })
#define SWAP32(val) ({ UINT32 x = val; x = (x << 24) | ((x << 8) & 0xff0000) | ((x >> 8) & 0xff00) | (x >> 24); x; })

#define SWAP8_R(addr)       *((UINT8 *)(addr))
#define SWAP8_W(val, addr)  *((UINT8 *)(addr)) = (val)
#define SWAP8_SEL_R(base, offset)      *((UINT8 *)((int)(base)+(int)(offset)))
#define SWAP8_SEL_W(val, base, offset) *((UINT8 *)((int)(base)+(int)(offset))) = (val)

/* WARNING: These macros *may* modify memory "in an unpredictable fashion" but they
 * do *not* have "memory" in the list of clobbered registers as it would reduce the
 * performance considerably. But as long as one don't first use a write macro to
 * change memory and then reads it *without* using a read macro, it should be safe.
 */
#define SWAP16_R(base) ({ UINT16 result; __asm__ volatile ("lhbrx %0,0,%1" : "=r" (result) : "r" (base)); result; })
#define SWAP32_R(base) ({ UINT32 result; __asm__ volatile ("lwbrx %0,0,%1" : "=r" (result) : "r" (base)); result; })
#define SWAP16_W(value, base) ({ __asm__ volatile ("sthbrx %0,0,%1" : : "r" (value), "r" (base)); })
#define SWAP32_W(value, base) ({ __asm__ volatile ("stwbrx %0,0,%1" : : "r" (value), "r" (base)); })

#define SWAP16_SEL_R(base, offset) ({ UINT16 result; __asm__ volatile ("lhbrx %0,%1,%2" : "=r" (result) : "b%" (offset), "r" (base)); result; })
#define SWAP32_SEL_R(base, offset) ({ UINT32 result; __asm__ volatile ("lwbrx %0,%1,%2" : "=r" (result) : "b%" (offset), "r" (base)); result; })
#define SWAP16_SEL_W(value, base, offset) ({ __asm__ volatile ("sthbrx %0,%1,%2" : : "r" (value), "b%" (offset), "r" (base)); })
#define SWAP32_SEL_W(value, base, offset) ({ __asm__ volatile ("stwbrx %0,%1,%2" : : "r" (value), "b%" (offset), "r" (base)); })

#elif __mc68000__

#define SWAP16(val) ({ UINT16 x = val; __asm__ volatile ("rol.w #8,%0" : "=d" (x) : "0" (x)); x; })
#define SWAP32(val) ({ UINT32 x = val; __asm__ volatile ("rol.w #8,%0;swap %0;rol.w #8,%0" : "=d" (x) : "0" (x)); x; })

#define SWAP8_R(addr)              *((UINT8  *)(addr))
#define SWAP16_R(addr)      SWAP16(*((UINT16 *)(addr)))
#define SWAP32_R(addr)      SWAP32(*((UINT32 *)(addr)))
#define SWAP8_W(val, addr)  *((UINT8  *)(addr)) = (val)
#define SWAP16_W(val, addr) *((UINT16 *)(addr)) = SWAP16(val)
#define SWAP32_W(val, addr) *((UINT32 *)(addr)) = SWAP32(val)

/* cast to int is used so the base pointer type doesn't confuse the compiler */

#define SWAP8_SEL_R(base, offset)              *((UINT8  *)((int)(base)+(int)(offset)))
#define SWAP16_SEL_R(base, offset)      SWAP16(*((UINT16 *)((int)(base)+(int)(offset))))
#define SWAP32_SEL_R(base, offset)      SWAP32(*((UINT32 *)((int)(base)+(int)(offset))))
#define SWAP8_SEL_W(val,  base, offset) *((UINT8  *)((int)(base)+(int)(offset))) = (val)
#define SWAP16_SEL_W(val, base, offset) *((UINT16 *)((int)(base)+(int)(offset))) = SWAP16(val)
#define SWAP32_SEL_W(val, base, offset) *((UINT32 *)((int)(base)+(int)(offset))) = SWAP32(val)

#else

#ifdef MSB_FIRST
#define SWAP16(val) ({ UINT16 x = val; x = (x << 8) | (x >> 8); x; })
#define SWAP32(val) ({ UINT32 x = val; x = (x << 24) | ((x << 8) & 0xff0000) | ((x >> 8) & 0xff00) | (x >> 24); x; })
#else
#define SWAP16(val) (val)
#define SWAP32(val) (val)
#endif

#define SWAP8_R(addr)              *((UINT8  *)(addr))
#define SWAP16_R(addr)      SWAP16(*((UINT16 *)(addr)))
#define SWAP32_R(addr)      SWAP32(*((UINT32 *)(addr)))
#define SWAP8_W(val, addr)  *((UINT8  *)(addr)) = (val)
#define SWAP16_W(val, addr) *((UINT16 *)(addr)) = SWAP16(val)
#define SWAP32_W(val, addr) *((UINT32 *)(addr)) = SWAP32(val)

/* cast to int is used so the base pointer type doesn't confuse the compiler */

#define SWAP8_SEL_R(base, offset)              *((UINT8  *)((int)(base)+(int)(offset)))
#define SWAP16_SEL_R(base, offset)      SWAP16(*((UINT16 *)((int)(base)+(int)(offset))))
#define SWAP32_SEL_R(base, offset)      SWAP32(*((UINT32 *)((int)(base)+(int)(offset))))
#define SWAP8_SEL_W(val,  base, offset) *((UINT8  *)((int)(base)+(int)(offset))) = (val)
#define SWAP16_SEL_W(val, base, offset) *((UINT16 *)((int)(base)+(int)(offset))) = SWAP16(val)
#define SWAP32_SEL_W(val, base, offset) *((UINT32 *)((int)(base)+(int)(offset))) = SWAP32(val)

#endif

#endif /* _FPSE_ENDIAN_H_ */
