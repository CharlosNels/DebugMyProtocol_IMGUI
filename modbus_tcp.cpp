#include "modbus_tcp.h"
#include "utils.h"


size_t Modbus_TCP::masterFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer)
{
    size_t index = 0;
    buffer[index++] = uint8_t(frame_info.trans_id >> 8 & 0xFF);
    buffer[index++] = uint8_t(frame_info.trans_id & 0xFF);
    buffer[index++] = 0;
    buffer[index++] = 0;
    uint8_t data_pack[512];
    size_t data_pack_index = 0;
    data_pack[data_pack_index++] = uint8_t(frame_info.id);
    data_pack[data_pack_index++] = uint8_t(frame_info.function);
    data_pack[data_pack_index++] = uint8_t(frame_info.reg_addr >> 8 & 0xFF);
    data_pack[data_pack_index++] = uint8_t(frame_info.reg_addr & 0xFF);
    if(frame_info.function != ModbusWriteSingleCoil &&
        frame_info.function != ModbusWriteSingleRegister)
    {
        data_pack[data_pack_index++] = uint8_t(frame_info.quantity >> 8 & 0xFF);
        data_pack[data_pack_index++] = uint8_t(frame_info.quantity & 0xFF);
    }
    if(frame_info.function == ModbusWriteSingleCoil ||
        frame_info.function == ModbusWriteSingleRegister)
    {
        data_pack[data_pack_index++] = uint8_t(frame_info.reg_values[0] >> 8 & 0xFF);
        data_pack[data_pack_index++] = uint8_t(frame_info.reg_values[0] & 0xFF);
    }
    if(frame_info.function == ModbusWriteMultipleCoils)
    {
        uint8_t byte_num = uint8_t(pageConvert(frame_info.quantity, 8));
        data_pack[data_pack_index++] = byte_num;
        uint8_t const *coils = (uint8_t const *)frame_info.reg_values;
        for(int i = 0;i < byte_num;++i)
        {
            data_pack[data_pack_index++] = coils[i];
        }
    }
    else if(frame_info.function == ModbusWriteMultipleRegisters)
    {
        data_pack[data_pack_index++] = uint8_t(frame_info.quantity *2);
        for(int i = 0;i <frame_info.quantity;++i)
        {
            data_pack[data_pack_index++] = uint8_t(frame_info.reg_values[i] >> 8 & 0xFF);
            data_pack[data_pack_index++] = uint8_t(frame_info.reg_values[i] & 0xFF);
        }
    }
    buffer[index++] = uint8_t(data_pack_index >> 8 & 0xFF);
    buffer[index++] = uint8_t(data_pack_index & 0xFF);
    memcpy(buffer + index, data_pack, data_pack_index);
    return index + data_pack_index;
}

ModbusFrameInfo Modbus_TCP::masterPack2Frame(const char *pack, size_t pack_size)
{
    ModbusFrameInfo ret{};
    ret.trans_id = uint16_t(pack[0]) << 8 | uint8_t(pack[1]);
    uint8_t data_pack[512];
    memcpy(data_pack, pack + 6, pack_size - 6);
    ret.id = uint8_t(data_pack[0]);
    ret.function = uint8_t(data_pack[1]);
    if(ret.function == ModbusReadCoils ||
        ret.function == ModbusReadDescreteInputs)
    {
        ret.quantity = uint8_t(data_pack[2]);
        uint8_t *coils = (uint8_t *)ret.reg_values;
        for(int i = 0;i < ret.quantity;++i)
        {
            coils[i] = data_pack[3 + i];
        }
    }
    else if(ret.function == ModbusReadHoldingRegisters ||
             ret.function == ModbusReadInputRegisters)
    {
        ret.quantity = uint8_t(data_pack[2]) / 2;
        for(int i = 0;i < ret.quantity;++i)
        {
            ret.reg_values[i] = uint16_t(data_pack[3 + 2 * i]) << 8 | uint8_t(data_pack[4 + 2 * i]);
        }
    }
    else if(ret.function == ModbusWriteSingleCoil ||
             ret.function == ModbusWriteSingleRegister)
    {
        ret.quantity = 1;
        unsigned char *coils = (unsigned char *)ret.reg_values;
        coils[0] = data_pack[4];
        coils[1] = data_pack[5];
    }
    else if(ret.function == ModbusWriteMultipleCoils
             || ret.function == ModbusWriteMultipleRegisters)
    {
        ret.quantity = uint16_t(data_pack[4]) << 8 | uint8_t(data_pack[5]);
    }
    else if(ret.function > ModbusFunctionError)
    {
        ret.reg_values[0] = data_pack[2];
    }
    else
    {
        // qDebug()<<"unknown modbus function : "<<ret.function;
    }
    return ret;
}

size_t Modbus_TCP::slaveFrame2Pack(const ModbusFrameInfo &frame_info, char *buffer)
{
    size_t index = 0;
    buffer[index++] = uint8_t(frame_info.trans_id >> 8 & 0xFF);
    buffer[index++] = uint8_t(frame_info.trans_id & 0xFF);
    buffer[index++] = 0;
    buffer[index++] = 0;
    uint8_t data_pack[512];
    size_t data_pack_index = 0;
    data_pack[data_pack_index++] = uint8_t(frame_info.id);
    data_pack[data_pack_index++] = uint8_t(frame_info.function);
    if(frame_info.function == ModbusCoilStatus ||
        frame_info.function == ModbusInputStatus)
    {
        uint8_t byte_num = uint8_t(pageConvert(frame_info.quantity, 8));
        data_pack[data_pack_index++] = byte_num;
        uint8_t const *coils = (uint8_t const *)frame_info.reg_values;
        for(int i = 0; i < byte_num; ++i)
        {
            data_pack[data_pack_index++] = coils[i];
        }
    }
    else if(frame_info.function == ModbusHoldingRegisters ||
             frame_info.function == ModbusInputRegisters)
    {
        uint8_t byte_num = uint8_t(frame_info.quantity << 1);
        data_pack[data_pack_index++] = byte_num;
        for(int i = 0;i < frame_info.quantity;++i)
        {
            data_pack[data_pack_index++] = uint8_t(frame_info.reg_values[i] >> 8 & 0xFF);
            data_pack[data_pack_index++] = uint8_t(frame_info.reg_values[i] & 0xFF);
        }
    }
    else if(frame_info.function == ModbusWriteSingleCoil ||
             frame_info.function == ModbusWriteSingleRegister)
    {
        data_pack[data_pack_index++] = uint8_t(frame_info.reg_addr >> 8 & 0xFF);
        data_pack[data_pack_index++] = uint8_t(frame_info.reg_addr & 0xFF);
        data_pack[data_pack_index++] = uint8_t(frame_info.reg_values[0] >> 8 & 0xFF);
        data_pack[data_pack_index++] = uint8_t(frame_info.reg_values[0] & 0xFF);
    }
    else if(frame_info.function == ModbusWriteMultipleCoils ||
             frame_info.function == ModbusWriteMultipleRegisters)
    {
        data_pack[data_pack_index++] = uint8_t(frame_info.reg_addr >> 8 & 0xFF);
        data_pack[data_pack_index++] = uint8_t(frame_info.reg_addr & 0xFF);
        data_pack[data_pack_index++] = uint8_t(frame_info.quantity >> 8 & 0xFF);
        data_pack[data_pack_index++] = uint8_t(frame_info.quantity & 0xFF);
    }
    else if(frame_info.function > ModbusFunctionError)
    {
        data_pack[data_pack_index++] = uint8_t(frame_info.reg_values[0] & 0xFF);
    }
    buffer[index++] = uint8_t(data_pack_index >> 8 & 0xFF);
    buffer[index++] = uint8_t(data_pack_index & 0xFF);
    memcpy(buffer + index, data_pack, data_pack_index);
    return index + data_pack_index;
}

ModbusFrameInfo Modbus_TCP::slavePack2Frame(const char *pack, size_t pack_size)
{
    ModbusFrameInfo ret{};
    ret.trans_id = uint16_t(pack[0]) << 8 | uint8_t(pack[1]);
    uint8_t data_pack[512];
    memcpy(data_pack, pack + 6, pack_size - 6);
    ret.id = uint8_t(data_pack[0]);
    ret.function = uint8_t(data_pack[1]);
    if(ret.function == ModbusReadCoils ||
        ret.function == ModbusReadDescreteInputs ||
        ret.function == ModbusReadHoldingRegisters ||
        ret.function == ModbusReadInputRegisters)
    {
        ret.reg_addr = uint16_t(data_pack[2]) << 8 | uint8_t(data_pack[3]);
        ret.quantity = uint16_t(data_pack[4]) << 8 | uint8_t(data_pack[5]);
    }
    else if(ret.function == ModbusWriteSingleCoil ||
             ret.function == ModbusWriteSingleRegister)
    {
        ret.reg_addr = uint16_t(data_pack[2]) << 8 | uint8_t(data_pack[3]);
        ret.quantity = 1;
        ret.reg_values[0] = uint16_t(data_pack[4]) << 8 | uint8_t(data_pack[5]);
    }
    else if(ret.function == ModbusWriteMultipleCoils)
    {
        ret.reg_addr = uint16_t(data_pack[2]) << 8 | uint8_t(data_pack[3]);
        ret.quantity = uint16_t(data_pack[4]) << 8 | uint8_t(data_pack[5]);
        int byte_num =  uint8_t(data_pack[6]);
        unsigned char *coils = (unsigned char *)ret.reg_values;
        for(int i = 0;i < byte_num;++i)
        {
            coils[i] = data_pack[7 + i];
        }
    }
    else if(ret.function == ModbusWriteMultipleRegisters)
    {
        ret.reg_addr = uint16_t(data_pack[2]) << 8 | uint8_t(data_pack[3]);
        ret.quantity = uint16_t(data_pack[4]) << 8 | uint8_t(data_pack[5]);
        for(int i = 0;i < ret.quantity; ++i)
        {
            ret.reg_values[i] = uint16_t(data_pack[7 + 2 * i]) << 8 | uint8_t(data_pack[8 + 2 * i]);
        }
    }
    return ret;
}

bool Modbus_TCP::validPack(const char *pack, size_t pack_size)
{
    uint16_t data_pack_size = uint16_t(pack[4]) << 8 | uint8_t(pack[5]);
    return data_pack_size == pack_size - 6;
}

Modbus_TCP::Modbus_TCP()
{}
