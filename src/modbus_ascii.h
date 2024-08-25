#ifndef MODBUS_ASCII_H
#define MODBUS_ASCII_H

#include "ModbusBase.h"

class Modbus_ASCII : public ModbusBase
{
public:
    size_t masterFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer);
    ModbusFrameInfo masterPack2Frame(const char *buffer, size_t buffer_size);
    size_t slaveFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer);
    ModbusFrameInfo slavePack2Frame(const char *buffer, size_t buffer_size);
    bool validPack(const char *buffer, size_t buffer_size);
    Modbus_ASCII();
private:
    static const char pack_start_character;
    static const char pack_terminator[];
};

#endif // MODBUS_ASCII_H
