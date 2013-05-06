// Copyright John MacCormick 2012. Modified BSD license. NO WARRANTY.
#pragma once

#include "windows.h"
#include <exception>

class win32_exception: public std::exception
{
public:
    typedef const void* Address; // OK on Win32 platform

    static void install_handler();
    virtual const char* what() const { return mWhat; };
    Address where() const { return mWhere; };
    unsigned code() const { return mCode; };
protected:
    win32_exception(const EXCEPTION_RECORD& info);
    static void __cdecl  translate(unsigned code, EXCEPTION_POINTERS* info);
private:
    const char* mWhat;
    Address mWhere;
    unsigned mCode;
};

class access_violation: public win32_exception
{
public:
    bool isWrite() const { return mIsWrite; };
    Address badAddress() const { return mBadAddress; };
private:
    bool mIsWrite;
    Address mBadAddress;
    access_violation(const EXCEPTION_RECORD& info);
    friend void win32_exception::translate(unsigned code, EXCEPTION_POINTERS* info);
};
