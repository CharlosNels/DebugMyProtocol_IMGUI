#ifndef MODBUSBASE_H
#define MODBUSBASE_H

#include "ModbusFrameInfo.h"
#include <stdlib.h>
class ModbusBase
{
public:
    ModbusBase(){};
    virtual bool validPack(const char *buffer, size_t buffer_size) = 0;
    virtual ModbusFrameInfo masterPack2Frame(const char *buffer, size_t buffer_size) = 0;
    virtual size_t masterFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer) = 0;
    virtual ModbusFrameInfo slavePack2Frame(const char *buffer, size_t buffer_size) = 0;
    virtual size_t slaveFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer) = 0;
};

#endif // MODBUSBASE_H
