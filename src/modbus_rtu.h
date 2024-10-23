#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#include "ModbusBase.h"

class Modbus_RTU : public ModbusBase {
  public:
    size_t masterFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer) override;
    ModbusFrameInfo masterPack2Frame(const char *buffer, size_t buffer_size) override;
    size_t slaveFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer) override;
    ModbusFrameInfo slavePack2Frame(const char *buffer, size_t buffer_size) override;
    bool validPack(const char *buffer, size_t buffer_size) override;
    Modbus_RTU();
};

#endif // MODBUS_RTU_H
