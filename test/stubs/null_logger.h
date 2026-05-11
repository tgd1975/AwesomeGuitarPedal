#pragma once
#include "i_logger.h"

class NullLogger : public ILogger
{
public:
    void log(const char*) override {}
    void log(const char*, const char*) override {}
};
