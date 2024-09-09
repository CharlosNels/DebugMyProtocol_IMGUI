#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include "ModbusBase.h"

class Modbus_RTU : public ModbusBase
{
public:
    size_t masterFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer);
    ModbusFrameInfo masterPack2Frame(const char *buffer, size_t buffer_size);
    size_t slaveFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer);
    ModbusFrameInfo slavePack2Frame(const char *buffer, size_t buffer_size);
    bool validPack(const char *buffer, size_t buffer_size);
    Modbus_RTU();
};

#endif // MODBUS_RTU_H
