#ifndef MODBUS_TCP_H
#define MODBUS_TCP_H

#include "ModbusBase.h"

/*
 * The format of modbus-udp packets is the same as that of modbus-tcp
 */

class Modbus_TCP : public ModbusBase {
  public:
    size_t masterFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer) override;
    ModbusFrameInfo masterPack2Frame(const char *buffer, size_t buffer_size) override;
    size_t slaveFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer) override;
    ModbusFrameInfo slavePack2Frame(const char *buffer, size_t buffer_size) override;
    bool validPack(const char *buffer, size_t buffer_size) override;
    Modbus_TCP();
};

#endif // MODBUS_TCP_H
