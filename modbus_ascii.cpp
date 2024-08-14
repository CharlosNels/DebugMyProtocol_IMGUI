#include "modbus_ascii.h"
#include "utils.h"
#include <stdio.h>


const char Modbus_ASCII::pack_start_character = ':';
const char Modbus_ASCII::pack_terminator[2] = {0x0D,0x0A};

size_t Modbus_ASCII::masterFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer)
{
    char pack[512];
    size_t index = 0;
    pack[index++] = uint8_t(frame_info.id);
    pack[index++] = uint8_t(frame_info.function);
    pack[index++] = uint8_t(frame_info.reg_addr >> 8 & 0xFF);
    pack[index++] = uint8_t(frame_info.reg_addr & 0XFF);
    if(frame_info.function != ModbusWriteSingleCoil &&
        frame_info.function != ModbusWriteSingleRegister)
    {
        pack[index++] = uint8_t(frame_info.quantity >> 8 & 0xFF);
        pack[index++] = uint8_t(frame_info.quantity & 0xFF);
    }
    if(frame_info.function == ModbusWriteSingleCoil ||
        frame_info.function == ModbusWriteSingleRegister)
    {
        pack[index++] = uint8_t(frame_info.reg_values[0] >> 8 & 0xFF);
        pack[index++] = uint8_t(frame_info.reg_values[0] & 0xFF);
    }
    if(frame_info.function == ModbusWriteMultipleCoils)
    {
        uint8_t byte_num = uint8_t(pageConvert(frame_info.quantity, 8));
        pack[index++] = byte_num;
        uint8_t const *coils = (uint8_t const *)frame_info.reg_values;
        for(int i = 0;i < byte_num;++i)
        {
            pack[index++] = coils[i];
        }
    }
    else if(frame_info.function == ModbusWriteMultipleRegisters)
    {
        pack[index++] = uint8_t(frame_info.quantity * 2);
        for(int i = 0;i <frame_info.quantity;++i)
        {
            pack[index++] = uint8_t(frame_info.reg_values[i] >> 8 & 0xFF);
            pack[index++] = uint8_t(frame_info.reg_values[i] & 0xFF);
        }
    }
    pack[index++] = LRC(buffer, index);
    buffer[0] = pack_start_character;
    size_t size = toHexString((const uint8_t *)pack, index, buffer + 1);
    size++;
    buffer[size++] = pack_terminator[0];
    buffer[size++] = pack_terminator[1];
    return size;
}

ModbusFrameInfo Modbus_ASCII::masterPack2Frame(const char *buffer, size_t size)
{
    ModbusFrameInfo ret{};
    uint8_t hex_pack[512];
    fromHexString(buffer + 1, size - 3, hex_pack);
    ret.id = uint8_t(hex_pack[0]);
    ret.function = uint8_t(hex_pack[1]);
    if(ret.function == ModbusReadCoils ||
        ret.function == ModbusReadDescreteInputs)
    {
        ret.quantity = uint8_t(hex_pack[2]);
        uint8_t *coils = (uint8_t *)ret.reg_values;
        for(int i = 0;i < ret.quantity;++i)
        {
            coils[i] = hex_pack[3 + i];
        }
    }
    else if(ret.function == ModbusReadHoldingRegisters ||
             ret.function == ModbusReadInputRegisters)
    {
        ret.quantity = uint8_t(hex_pack[2]) / 2;
        for(int i = 0;i < ret.quantity;++i)
        {
            ret.reg_values[i] = uint16_t(hex_pack[3 + 2 * i]) << 8 | uint8_t(hex_pack[4 + 2 * i]);
        }
    }
    else if(ret.function == ModbusWriteSingleCoil ||
             ret.function == ModbusWriteSingleRegister)
    {
        ret.quantity = 1;
        uint8_t *coils = (uint8_t *)ret.reg_values;
        coils[0] = hex_pack[4];
        coils[1] = hex_pack[5];
    }
    else if(ret.function == ModbusWriteMultipleCoils
             || ret.function == ModbusWriteMultipleRegisters)
    {
        ret.quantity = uint16_t(hex_pack[4]) << 8 | uint8_t(hex_pack[5]);
    }
    else if(ret.function > ModbusFunctionError)
    {
        ret.reg_values[0] = hex_pack[2];
    }
    else
    {
        //Error
    }
    return ret;
}

size_t Modbus_ASCII::slaveFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer)
{
    uint8_t pack[512];
    size_t index = 0;
    pack[index++] = uint8_t(frame_info.id);
    pack[index++] = uint8_t(frame_info.function);
    if(frame_info.function == ModbusCoilStatus ||
        frame_info.function == ModbusInputStatus)
    {
        uint8_t byte_num = uint8_t(pageConvert(frame_info.quantity, 8));
        pack[index++] = byte_num;
        uint8_t const *coils = (uint8_t const *)frame_info.reg_values;
        for(int i = 0; i < byte_num; ++i)
        {
            pack[index++] = coils[i];
        }
    }
    else if(frame_info.function == ModbusHoldingRegisters ||
             frame_info.function == ModbusInputRegisters)
    {
        uint8_t byte_num = uint8_t(frame_info.quantity << 1);
        pack[index++] = byte_num;
        for(int i = 0;i < frame_info.quantity;++i)
        {
            pack[index++] = uint8_t(frame_info.reg_values[i] >> 8 & 0xFF);
            pack[index++] = uint8_t(frame_info.reg_values[i] & 0xFF);
        }
    }
    else if(frame_info.function == ModbusWriteSingleCoil ||
             frame_info.function == ModbusWriteSingleRegister)
    {
        pack[index++] = uint8_t(frame_info.reg_addr >> 8 & 0xFF);
        pack[index++] = uint8_t(frame_info.reg_addr & 0xFF);
        pack[index++] = uint8_t(frame_info.reg_values[0] >> 8 & 0xFF);
        pack[index++] = uint8_t(frame_info.reg_values[0] & 0xFF);
    }
    else if(frame_info.function == ModbusWriteMultipleCoils ||
             frame_info.function == ModbusWriteMultipleRegisters)
    {
        pack[index++] = uint8_t(frame_info.reg_addr >> 8 & 0xFF);
        pack[index++] = uint8_t(frame_info.reg_addr & 0xFF);
        pack[index++] = uint8_t(frame_info.quantity >> 8 & 0xFF);
        pack[index++] = uint8_t(frame_info.quantity & 0xFF);
    }
    else if(frame_info.function > ModbusFunctionError)
    {
        pack[index++] = uint8_t(frame_info.reg_values[0] >> 8 & 0xFF);
    }
    pack[index++] = LRC(buffer, index);
    buffer[0] = pack_start_character;
    size_t size = toHexString((const uint8_t *)pack, index, buffer + 1);
    size++;
    buffer[size++] = pack_terminator[0];
    buffer[size++] = pack_terminator[1];
    return size;
}

ModbusFrameInfo Modbus_ASCII::slavePack2Frame(const char *buffer, size_t buffer_size)
{
    ModbusFrameInfo ret{};
    uint8_t hex_pack[512];
    fromHexString(buffer + 1, buffer_size - 3, hex_pack);
    ret.id = uint8_t(hex_pack[0]);
    ret.function = uint8_t(hex_pack[1]);
    if(ret.function == ModbusReadCoils ||
        ret.function == ModbusReadDescreteInputs ||
        ret.function == ModbusReadHoldingRegisters ||
        ret.function == ModbusReadInputRegisters)
    {
        ret.reg_addr = uint16_t(hex_pack[2]) << 8 | uint8_t(hex_pack[3]);
        ret.quantity = uint16_t(hex_pack[4]) << 8 | uint8_t(hex_pack[5]);
    }
    else if(ret.function == ModbusWriteSingleCoil ||
             ret.function == ModbusWriteSingleRegister)
    {
        ret.reg_addr = uint16_t(hex_pack[2]) << 8 | uint8_t(hex_pack[3]);
        ret.quantity = 1;
        ret.reg_values[0] = uint16_t(hex_pack[4]) << 8 | uint8_t(hex_pack[5]);
    }
    else if(ret.function == ModbusWriteMultipleCoils)
    {
        ret.reg_addr = uint16_t(hex_pack[2]) << 8 | uint8_t(hex_pack[3]);
        ret.quantity = uint16_t(hex_pack[4]) << 8 | uint8_t(hex_pack[5]);
        int byte_num =  uint8_t(hex_pack[6]);
        uint8_t *coils = (uint8_t *)ret.reg_values;
        for(int i = 0;i < byte_num;++i)
        {
            coils[i] = hex_pack[7 + i];
        }
    }
    else if(ret.function == ModbusWriteMultipleRegisters)
    {
        ret.reg_addr = uint16_t(hex_pack[2]) << 8 | uint8_t(hex_pack[3]);
        ret.quantity = uint16_t(hex_pack[4]) << 8 | uint8_t(hex_pack[5]);
        for(int i = 0;i < ret.quantity; ++i)
        {
            ret.reg_values[i] = uint16_t(hex_pack[7 + 2 * i]) << 8 | uint8_t(hex_pack[8 + i * 2]);
        }
    }
    return ret;
}

bool Modbus_ASCII::validPack(const char *buffer, size_t buffer_size)
{
    uint8_t hex_pack[512];
    size_t size = fromHexString(buffer + 1, buffer_size - 3, hex_pack);
    return buffer[0] == pack_start_character && buffer[buffer_size -2] == pack_terminator[0] && buffer[buffer_size -1] == pack_terminator[1] && (LRC((const char *)hex_pack, size) == 0);
}

Modbus_ASCII::Modbus_ASCII()
{}
