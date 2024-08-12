//
//  shared_definitions.h
//  OneBillionRows
//
//  Created by Badger on 1/8/2024.
//

#ifndef shared_definitions_h
#define shared_definitions_h

#define likely(x)    __builtin_expect(!!(x), 1)
#define unlikely(x)  __builtin_expect(!!(x), 0)

// Custom definitions as it makes compiling on multiple OSes
// easier.
#define MYMAX( a, b ) ( ( a > b) ? a : b )
#define MYMIN( a, b ) ( ( a < b) ? a : b )

#endif /* shared_definitions_h */
