
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

// TODO: Any benefit in making this a template or abstract struct? I think it will end up being 2 functions pointers anyway?

struct Chainfunction {
        
    void (*callback)(void);
    
    void *(next)(void);
        
    Chainfunction( void (*cb)(void) ) {
     
        callback = cb;   
                
    }        
    
};


#endif /* CHAINFUNCTION_H_ */

 
 