/*
 * version.h
 */

#ifndef _VERSION_H_
#define _VERSION_H_

extern const char vstring[];
extern const char verstag[];

#define VERSTAG &verstag[1]
#define VERNAME &verstag[7]
#define VERSION &verstag[14]

#endif /* _VERSION_H_ */
