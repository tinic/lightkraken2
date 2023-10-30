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
#ifndef _SPI_H
#define _SPI_H

#include <stdint.h>
#include <stdlib.h>

class SPI {
   public:
    virtual void transfer(const uint8_t *buf, size_t len, bool wantsSCLK) = 0;
    virtual void update() = 0;
    virtual bool busy() const = 0;
    void setFast(bool state) {};

   protected:
    virtual ~SPI(){};
    bool initialized = false;
    virtual void init() = 0;
};

class SPI_0 : public SPI {
   public:
    static SPI &instance();
    virtual void transfer(const uint8_t *buf, size_t len, bool wantsSCLK) override;
    virtual void update() override;
    virtual bool busy() const override;

   protected:
    virtual ~SPI_0(){};
    bool initialized = false;
    virtual void init() override;
};

class SPI_1 : public SPI {
   public:
    static SPI &instance();
    virtual void transfer(const uint8_t *buf, size_t len, bool wantsSCLK) override;
    virtual void update() override;
    virtual bool busy() const override;

   protected:
    virtual ~SPI_1(){};
    bool initialized = false;
    virtual void init() override;
};

#endif  // #ifndef _SPI_H
