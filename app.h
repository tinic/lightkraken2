
#ifndef _APP_H_
#define _APP_H_

#include <stdint.h>


class App {
   public:
    static App &instance();

    void scheduleReset(int32_t count = 2000) { resetCount = count; };
    void checkReset();

   private:
    void init();
    bool initialized = false;

    int32_t resetCount = 0;
};

#endif  // #ifndef _APP_H_
