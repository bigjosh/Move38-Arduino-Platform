
// Defines a template that is used to add chainable function calls to a handler
// This should be statically allocated in the module that is adding the chain
// Otherwise the handler would either need to...
//
//      (1) allocate a fixed number of slots that could run out, or
//      (2) dynamically allocate space each time a new function is added, but better to see how much space used at compile time.

#ifndef CHAINFUNCTION_H_
#define CHAINFUNCTION_H_

#include <stddef.h>

typedef void *voidvoidfunction_ptr(void);

typedef struct chainfunction_struct {

    void (*callback)(void);

    struct chainfunction_struct *next;              // Sorry for this ugly syntax, this is how we do it in old skool C! https://stackoverflow.com/questions/18582205/linkedlist-struct-typedef-in-c

} chainfunction_t ;

#endif /* CHAINFUNCTION_H_ */


