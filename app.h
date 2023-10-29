
#ifndef _APP_H_
#define _APP_H_

#include <stdint.h>

class App {
   public:
    static App &instance();
    void start();
    void setup(void *first_unused_memory);
   private:
    void init();
    bool initialized = false;
};

#endif  // #ifndef _APP_H_
