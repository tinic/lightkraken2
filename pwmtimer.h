/*
Copyright 2019 Tinic Uro

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef _PWM_TIMER_H
#define _PWM_TIMER_H

#include <stdint.h>

class PwmTimer {
   public:
    constexpr static uint16_t initPulse = 0x7fff;
    constexpr static uint16_t pwmPeriod = 0xffff;

    virtual void setPulse(uint16_t pulse) = 0;
    virtual void start() = 0;
    virtual void stop() = 0;

   protected:
    virtual ~PwmTimer(){};
    bool initialized = false;
    virtual void init() = 0;
};

class PwmTimer0 : public PwmTimer {
   public:
    static PwmTimer &instance();
    virtual void setPulse(uint16_t pulse) override;
    virtual void start() override;
    virtual void stop() override;

   private:
    virtual void init() override;
};

class PwmTimer1 : public PwmTimer {
   public:
    static PwmTimer &instance();
    virtual void setPulse(uint16_t pulse) override;
    virtual void start() override;
    virtual void stop() override;

   private:
    virtual void init() override;
};

class PwmTimer2 : public PwmTimer {
   public:
    static PwmTimer &instance();
    virtual void setPulse(uint16_t pulse) override;
    virtual void start() override;
    virtual void stop() override;

   private:
    virtual void init() override;
};

class PwmTimer3 : public PwmTimer {
   public:
    static PwmTimer &instance();
    virtual void setPulse(uint16_t pulse) override;
    virtual void start() override;
    virtual void stop() override;

   private:
    virtual void init() override;
};

class PwmTimer4 : public PwmTimer {
   public:
    static PwmTimer &instance();
    virtual void setPulse(uint16_t pulse) override;
    virtual void start() override;
    virtual void stop() override;

   private:
    virtual void init() override;
};

class PwmTimer5 : public PwmTimer {
   public:
    static PwmTimer &instance();
    virtual void setPulse(uint16_t pulse) override;
    virtual void start() override;
    virtual void stop() override;

   private:
    virtual void init() override;
};

#endif  // #ifndef _PWM_TIMER_H