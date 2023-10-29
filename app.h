
#ifndef _APP_H_
#define _APP_H_

#include <stdint.h>

class App {
   public:
    static App &instance();
   private:
    void init();
    bool initialized = false;
};

#endif  // #ifndef _APP_H_
