#include "modbus_rtu.h"
#include "utils.h"

size_t Modbus_RTU::masterFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer) {
    size_t index = 0;
    buffer[index++] = uint8_t(frame_info.id);
    buffer[index++] = uint8_t(frame_info.function);
    buffer[index++] = uint8_t(frame_info.reg_addr >> 8 & 0xFF);
    buffer[index++] = uint8_t(frame_info.reg_addr & 0xFF);
    if (frame_info.function != ModbusWriteSingleCoil && frame_info.function != ModbusWriteSingleRegister) {
        buffer[index++] = uint8_t(frame_info.quantity >> 8 & 0xFF);
        buffer[index++] = uint8_t(frame_info.quantity & 0xFF);
    }
    if (frame_info.function == ModbusWriteSingleCoil || frame_info.function == ModbusWriteSingleRegister) {
        buffer[index++] = uint8_t(frame_info.reg_values[0] >> 8 & 0xFF);
        buffer[index++] = uint8_t(frame_info.reg_values[0] & 0xFF);
    }
    if (frame_info.function == ModbusWriteMultipleCoils) {
        uint8_t byte_num = uint8_t(pageConvert(frame_info.quantity, 8));
        buffer[index++] = byte_num;
        uint8_t const *coils = (uint8_t const *)frame_info.reg_values;
        for (int i = 0; i < byte_num; ++i) {
            buffer[index++] = coils[i];
        }
    } else if (frame_info.function == ModbusWriteMultipleRegisters) {
        buffer[index++] = uint8_t(frame_info.quantity * 2);
        for (int i = 0; i < frame_info.quantity; ++i) {
            buffer[index++] = uint8_t(frame_info.reg_values[i] >> 8 & 0xFF);
            buffer[index++] = uint8_t(frame_info.reg_values[i] & 0xFF);
        }
    }
    uint16_t crc_value = CRC_16(buffer, index);
    buffer[index++] = uint8_t(crc_value & 0xFF);
    buffer[index++] = uint8_t(crc_value >> 8 & 0xFF);
    return index;
}

ModbusFrameInfo Modbus_RTU::masterPack2Frame(const char *buffer, size_t buffer_size) {
    ModbusFrameInfo ret;
    ret.id = uint8_t(buffer[0]);
    ret.function = uint8_t(buffer[1]);
    if (ret.function == ModbusReadCoils || ret.function == ModbusReadDescreteInputs) {
        ret.quantity = uint8_t(buffer[2]);
        uint8_t *coils = (uint8_t *)ret.reg_values;
        for (int i = 0; i < ret.quantity; ++i) {
            coils[i] = buffer[3 + i];
        }
    } else if (ret.function == ModbusReadHoldingRegisters || ret.function == ModbusReadInputRegisters) {
        ret.quantity = uint8_t(buffer[2]) / 2;
        for (int i = 0; i < ret.quantity; ++i) {
            ret.reg_values[i] = uint16_t(buffer[3 + 2 * i]) << 8 | uint8_t(buffer[4 + 2 * i]);
        }
    } else if (ret.function == ModbusWriteSingleCoil || ret.function == ModbusWriteSingleRegister) {
        ret.quantity = 1;
        uint8_t *coils = (uint8_t *)ret.reg_values;
        coils[0] = buffer[4];
        coils[1] = buffer[5];
    } else if (ret.function == ModbusWriteMultipleCoils || ret.function == ModbusWriteMultipleRegisters) {
        ret.quantity = uint16_t(buffer[4]) << 8 | uint8_t(buffer[5]);
    } else if (ret.function > ModbusFunctionError) {
        ret.reg_values[0] = buffer[2];
    } else {
        // qDebug()<<"unknown modbus function : "<<ret.function;
    }
    return ret;
}

size_t Modbus_RTU::slaveFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer) {
    size_t index = 0;
    buffer[index++] = uint8_t(frame_info.id);
    buffer[index++] = uint8_t(frame_info.function);
    if (frame_info.function == ModbusCoilStatus || frame_info.function == ModbusInputStatus) {
        uint8_t byte_num = uint8_t(pageConvert(frame_info.quantity, 8));
        buffer[index++] = byte_num;
        uint8_t const *coils = (uint8_t const *)frame_info.reg_values;
        for (int i = 0; i < byte_num; ++i) {
            buffer[index++] = coils[i];
        }
    } else if (frame_info.function == ModbusHoldingRegisters || frame_info.function == ModbusInputRegisters) {
        uint8_t byte_num = uint8_t(frame_info.quantity << 1);
        buffer[index++] = byte_num;
        for (int i = 0; i < frame_info.quantity; ++i) {
            buffer[index++] = uint8_t(frame_info.reg_values[i] >> 8 & 0xFF);
            buffer[index++] = uint8_t(frame_info.reg_values[i] & 0xFF);
        }
    } else if (frame_info.function == ModbusWriteSingleCoil || frame_info.function == ModbusWriteSingleRegister) {
        buffer[index++] = uint8_t(frame_info.reg_addr >> 8 & 0xFF);
        buffer[index++] = uint8_t(frame_info.reg_addr & 0xFF);
        buffer[index++] = uint8_t(frame_info.reg_values[0] >> 8 & 0xFF);
        buffer[index++] = uint8_t(frame_info.reg_values[0] & 0xFF);
    } else if (frame_info.function == ModbusWriteMultipleCoils || frame_info.function == ModbusWriteMultipleRegisters) {
        buffer[index++] = uint8_t(frame_info.reg_addr >> 8 & 0xFF);
        buffer[index++] = uint8_t(frame_info.reg_addr & 0xFF);
        buffer[index++] = uint8_t(frame_info.quantity >> 8 & 0xFF);
        buffer[index++] = uint8_t(frame_info.quantity & 0xFF);
    } else if (frame_info.function > ModbusFunctionError) {
        buffer[index++] = uint8_t(frame_info.reg_values[0] & 0xFF);
    }
    uint16_t crc_value = CRC_16(buffer, index);
    buffer[index++] = uint8_t(crc_value & 0xFF);
    buffer[index++] = uint8_t(crc_value >> 8 & 0xFF);
    return index;
}

ModbusFrameInfo Modbus_RTU::slavePack2Frame(const char *pack, size_t pack_size) {
    ModbusFrameInfo ret;
    ret.id = uint8_t(pack[0]);
    ret.function = uint8_t(pack[1]);
    if (ret.function == ModbusReadCoils || ret.function == ModbusReadDescreteInputs ||
        ret.function == ModbusReadHoldingRegisters || ret.function == ModbusReadInputRegisters) {
        ret.reg_addr = uint16_t(pack[2]) << 8 | uint8_t(pack[3]);
        ret.quantity = uint16_t(pack[4]) << 8 | uint8_t(pack[5]);
    } else if (ret.function == ModbusWriteSingleCoil || ret.function == ModbusWriteSingleRegister) {
        ret.reg_addr = uint16_t(pack[2]) << 8 | uint8_t(pack[3]);
        ret.quantity = 1;
        ret.reg_values[0] = uint16_t(pack[4]) << 8 | uint8_t(pack[5]);
    } else if (ret.function == ModbusWriteMultipleCoils) {
        ret.reg_addr = uint16_t(pack[2]) << 8 | uint8_t(pack[3]);
        ret.quantity = uint16_t(pack[4]) << 8 | uint8_t(pack[5]);
        int byte_num = uint8_t(pack[6]);
        uint8_t *coils = (uint8_t *)ret.reg_values;
        for (int i = 0; i < byte_num; ++i) {
            coils[i] = pack[7 + i];
        }
    } else if (ret.function == ModbusWriteMultipleRegisters) {
        ret.reg_addr = uint16_t(pack[2]) << 8 | uint8_t(pack[3]);
        ret.quantity = uint16_t(pack[4]) << 8 | uint8_t(pack[5]);
        for (int i = 0; i < ret.quantity; ++i) {
            ret.reg_values[i] = uint16_t(pack[7 + 2 * i]) << 8 | uint8_t(pack[8 + 2 * i]);
        }
    }
    return ret;
}

bool Modbus_RTU::validPack(const char *buffer, size_t buffer_size) { return CRC_16(buffer, buffer_size) == 0; }

Modbus_RTU::Modbus_RTU() {}
