#ifndef __MODBUSWINDOW_H__
#define __MODBUSWINDOW_H__

#include "ModbusFrameInfo.h"
#include "MyIODevice.h"
#include "ModbusBase.h"
#include <cstdio>
#include <cstring>
#include <stdint.h>
#include <vector>
#include <string>
#include <list>
#include <SDL.h>
#include <unordered_map>
#include <string>
#include <stdlib.h>
#include "utils.h"

#define REGISTER_ALIAS_MAX_LEN 64

enum CellFormat
{
    Format_None = 0,
    Format_Coil,
    Format_Signed,
    Format_Unsigned,
    Format_Hex,
    Format_Ascii_Hex,
    Format_Binary,
    Format_32_Bit_Signed_Big_Endian = 32,
    Format_32_Bit_Signed_Little_Endian,
    Format_32_Bit_Signed_Big_Endian_Byte_Swap,
    Format_32_Bit_Signed_Little_Endian_Byte_Swap,
    Format_32_Bit_Unsigned_Big_Endian,
    Format_32_Bit_Unsigned_Little_Endian,
    Format_32_Bit_Unsigned_Big_Endian_Byte_Swap,
    Format_32_Bit_Unsigned_Little_Endian_Byte_Swap,
    Format_32_Bit_Float_Big_Endian,
    Format_32_Bit_Float_Little_Endian,
    Format_32_Bit_Float_Big_Endian_Byte_Swap,
    Format_32_Bit_Float_Little_Endian_Byte_Swap,
    Format_64_Bit_Signed_Big_Endian = 64,
    Format_64_Bit_Signed_Little_Endian,
    Format_64_Bit_Signed_Big_Endian_Byte_Swap,
    Format_64_Bit_Signed_Little_Endian_Byte_Swap,
    Format_64_Bit_Unsigned_Big_Endian,
    Format_64_Bit_Unsigned_Little_Endian,
    Format_64_Bit_Unsigned_Big_Endian_Byte_Swap,
    Format_64_Bit_Unsigned_Little_Endian_Byte_Swap,
    Format_64_Bit_Float_Big_Endian,
    Format_64_Bit_Float_Little_Endian,
    Format_64_Bit_Float_Big_Endian_Byte_Swap,
    Format_64_Bit_Float_Little_Endian_Byte_Swap,
};

struct RegistersTableData
{
    ModbusIdentifier identifier;
    char table_title[128]{0};
    char info[128]{0};
    char msg[128]{0};
    uint16_t id{0};
    uint16_t reg_start{0};
    uint16_t reg_end{0};
    uint16_t reg_quantity{0};
    char **reg_alias{nullptr};
    bool *reg_alias_selected{nullptr};
    uint16_t *reg_values{nullptr};
    bool *reg_values_selected{nullptr};
    uint32_t send_count{0};
    uint32_t error_count{0};
    uint8_t function{0};
    uint32_t scan_rate{1000};
    char packet[512];
    uint16_t packet_size{0};
    CellFormat *cell_formats{nullptr};
    bool table_visible{true};
    RegistersTableData(uint16_t _id,uint16_t _reg_start, uint16_t _reg_end, uint8_t _function, uint32_t _scan_rate, ModbusIdentifier _identifier) :
    identifier(_identifier), id(_id), reg_start(_reg_start), reg_end(_reg_end), reg_quantity(_reg_end - _reg_start + 1), function(_function), scan_rate(_scan_rate)
    {
        update_info();
        reg_values = new uint16_t[reg_quantity];
        memset(reg_values, 0, reg_quantity * sizeof(uint16_t));
        reg_values_selected = new bool[reg_quantity];
        cell_formats = new CellFormat[reg_quantity];
        reg_alias = new char*[reg_quantity];
        reg_alias_selected = new bool[reg_quantity];
        CellFormat default_fmt = (function == ModbusReadCoils || function == ModbusWriteSingleCoil || function == ModbusReadDescreteInputs || function == ModbusWriteMultipleCoils) ? Format_Coil : Format_Unsigned;
        for(int i = 0;i < reg_quantity; ++i)
        {
            reg_values_selected[i] = false;
            cell_formats[i] = default_fmt;
            reg_alias[i] = new char[REGISTER_ALIAS_MAX_LEN];
            memset(reg_alias[i], 0, REGISTER_ALIAS_MAX_LEN);
            reg_alias_selected[i] = false;
        }
    }

    void update_info()
    {
        if(identifier == ModbusMaster)
        {
            snprintf(info, sizeof(info), "Tx=%u;Err=%u;ID=%u;F=%02u;SR=%ums", send_count, error_count, id, function, scan_rate);
        }
        else
        {
            snprintf(info, sizeof(info), "ID=%u;F=%02u", id, function);
        }
    }

    void modify_registers()
    {
        delete[] reg_values;
        delete[] reg_values_selected;
        delete[] cell_formats;
        delete[] reg_alias;
        delete[] reg_alias_selected;
        reg_values = new uint16_t[reg_quantity];
        memset(reg_values, 0, reg_quantity * sizeof(reg_values[0]));
        reg_values_selected = new bool[reg_quantity];
        cell_formats = new CellFormat[reg_quantity];
        reg_alias = new char*[reg_quantity];
        reg_alias_selected = new bool[reg_quantity];
        for(int i = 0; i < reg_quantity; ++i)
        {
            reg_values_selected[i] = false;
            cell_formats[i] = Format_Unsigned;
            reg_alias[i] = new char[REGISTER_ALIAS_MAX_LEN];
            memset(reg_alias[i], 0, REGISTER_ALIAS_MAX_LEN);
            reg_alias_selected[i] = false;
        }
    }

    ~RegistersTableData()
    {
        delete[] reg_values;
        delete[] cell_formats;
        for(int i = 0; i < reg_quantity; ++i)
        {
            delete[] reg_alias[i];
        }
        delete[] reg_alias;
        delete[] reg_alias_selected;
        delete[] reg_values_selected;
    }
};

struct CommunicationTrafficWindowData
{
    bool stopped{false};
    std::string communication_traffic_text;
    bool stop_on_error{false};
    bool timestamp{false};
};

struct RegistersDefineData
{
    int id{0};
    ComboBoxData function_combo_box_data;
    int reg_addr{0};
    int quantity{0};
    int scan_rate{1000};
    ModbusFrameInfo frame_info{};
    char hex_str[512], packet[512];
    size_t packet_size;
};

struct ModbusPacket{
    char packet[512];
    size_t packet_size;
};

struct Function_05_06_Data
{
    int slave_id{0};
    int address{0};
    int value{0};
    int function{5};
    char packet[512]{0};
    size_t packet_size;
    char hex_str[512]{0};
};

union Value_Data
{
    bool bool_value;
    int32_t s32_value;
    int64_t s64_value;
    float f32_value;
    double f64_value;
};


struct Function_15_16_Data
{
    int slave_id{0};
    int address{0};
    int quantity{0};
    Value_Data values[MODBUS_FRAME_MAX_REGISTERS]{ 0 };
    char value_name[MODBUS_FRAME_MAX_REGISTERS][8]{ 0 };
    CellFormat format{Format_Unsigned};
    ComboBoxData format_combo_box_data{};
};

class ModbusWindow
{
    friend uint32_t Scan_timer_callback(uint32_t interval, void *param);
    friend uint32_t Send_timer_callback(uint32_t interval, void *param);
    friend uint32_t Recv_timer_callback(uint32_t interval, void *param);
public:
    ModbusWindow(MyIODevice *myIODevice, const char *window_name, ModbusIdentifier identifier, Protocols protocol, ModbusBase *modbus_base);

    ~ModbusWindow();
    void render();

    bool visible(){ return m_visible; }

private:
    void render_menu_bar();

    void render_master_menu();

    void render_slave_menu();

    void render_registers_tables();

    void render_add_registers_dialog();

    void render_modify_registers_dialog();

    void render_communication_traffic_dialog();

    void render_error_counter_dialog();

    void render_timeout_setting_dialog();

    void render_modbus_function_05_dialog();

    void render_modbus_function_06_dialog();

    void render_modbus_function_15_dialog();

    void render_modbus_function_16_dialog();

    void get_value_by_format(CellFormat format, uint16_t *value_ptr, char *value_str, int max_len);

    void set_value_by_format(CellFormat format, uint16_t *value_ptr, const char *value_str);

    void read_data_callback(const char *buffer, size_t size);

    uint32_t scan_timer_callback(uint32_t interval, void *param);

    uint32_t send_timer_callback(uint32_t interval, void *param);

    uint32_t recv_timer_callback(uint32_t interval, void *param);

    void error_handle(const char *error_msg);

    void write_master_register_value(CellFormat format, const char *value_str, RegistersTableData *reg_table_data, int reg_index);

    void process_master_frame(const ModbusFrameInfo &frame_info);

    void process_slave_frame(const ModbusFrameInfo &frame_info, RegistersTableData *slave_reg_table_data, ModbusErrorCode &error_code);

    RegistersTableData *getSlaveReadTableData(int id, int function, int reg_start, int reg_end, ModbusErrorCode &error_code);

private:

    MyIODevice *m_myIODevice;
    char m_window_name[128];
    bool m_visible;
    ModbusIdentifier m_identifier;
    Protocols m_protocol;

    bool m_add_registers_dialog_visible;
    bool m_modify_registers_dialog_visible;
    bool m_communication_traffic_dialog_visible;
    bool m_error_counter_dialog_visible;
    bool m_timeout_setting_dialog_visible;
    bool m_modbus_function_05_dialog_visible;
    bool m_modbus_function_06_dialog_visible;
    bool m_modbus_function_15_dialog_visible;
    bool m_modbus_function_16_dialog_visible;

    std::vector<RegistersTableData*> m_registers_table_datas;
    std::list<ModbusPacket*> m_cycle_list;
    std::list<ModbusPacket*> m_manual_list;
    std::list<RegistersTableData*> m_cycle_table_list;

    ModbusPacket *m_master_last_send_data;
    ModbusFrameInfo m_master_last_send_frame;
    ModbusBase *m_modbus;

    SDL_TimerID m_scan_timer_id;
    SDL_TimerID m_send_timer_id;
    SDL_TimerID m_recv_timer_id;

    std::unordered_map<RegistersTableData*, uint32_t> m_last_scan_timestamp_map;
    std::unordered_map<const char *, ModbusFunctions> modbus_function_map;
    std::unordered_map<const char *, ModbusFunctions> modbus_slave_function_map;
    std::unordered_map<ModbusErrorCode, const char *> modbus_error_code_map;
    std::unordered_map<ModbusErrorCode, const char *> modbus_error_code_comment_map;
    std::unordered_map<ModbusErrorCode, uint32_t> m_error_count_map;
    std::unordered_map<const char *, CellFormat> m_write_format_map;

    uint16_t m_trans_id;
    uint32_t m_recv_timeout_ms;

    std::function<void(ModbusErrorCode)> m_write_frame_response_callback;
    CommunicationTrafficWindowData m_communication_traffic_window_data;
    RegistersDefineData m_add_reg_def_data;
    RegistersDefineData m_modify_reg_def_data;
    ComboBoxData m_modify_table_names_combo_box_data;
    const char *m_last_selected_table_name;
    int m_tmp_recv_timeout_ms;
    bool m_close_on_resp_ok;
    Function_05_06_Data m_function_05_data;
    Function_05_06_Data m_function_06_data;
    Function_15_16_Data m_function_15_data;
    Function_15_16_Data m_function_16_data;
    ModbusFrameInfo m_write_frame_info;
};

uint32_t Scan_timer_callback(uint32_t interval, void *param);

uint32_t Send_timer_callback(uint32_t interval, void *param);

uint32_t Recv_timer_callback(uint32_t interval, void *param);

#endif // __MODBUSWINDOW_H__
