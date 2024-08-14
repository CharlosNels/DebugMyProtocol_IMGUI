#include "ModbusWindow.h"
#include "MainWindow.h"
#include "ModbusBase.h"
#include "ModbusFrameInfo.h"
#include "ImGui_CustomWidgets.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <imgui.h>
#include <libintl.h>
#include <fstream>
#include <mutex>

ModbusWindow::ModbusWindow(MyIODevice *myIODevice, const char *window_name, ModbusIdentifier identifier, Protocols protocol, ModbusBase *modbus_base)
    : m_myIODevice(myIODevice), m_visible(true), m_identifier(identifier), m_protocol(protocol), m_add_registers_dialog_visible(false),
      m_modify_registers_dialog_visible(false), m_communication_traffic_dialog_visible(false),
      m_error_counter_dialog_visible(false), m_timeout_setting_dialog_visible(false),
      m_modbus_function_05_dialog_visible(false), m_modbus_function_06_dialog_visible(false),
      m_modbus_function_15_dialog_visible(false), m_modbus_function_16_dialog_visible(false),
      m_master_last_send_data(nullptr), m_modbus(modbus_base), m_trans_id(0), m_recv_timeout_ms(300),
      m_last_selected_table_name(nullptr), m_tmp_recv_timeout_ms(m_recv_timeout_ms)
{
    strcpy(m_window_name, window_name);
    m_myIODevice->setReadDataCallback(std::bind(&ModbusWindow::read_data_callback, this, std::placeholders::_1, std::placeholders::_2));
    modbus_function_map = {
        {gettext("01 Read Coils (0x)"), ModbusReadCoils},
        {gettext("02 Read Descrete Inputs (1x)"), ModbusReadDescreteInputs},
        {gettext("03 Read Holding Registers (4x)"), ModbusReadHoldingRegisters},
        {gettext("04 Read Input Registers (3x)"), ModbusReadInputRegisters},
        {gettext("05 Write Single Coil"), ModbusWriteSingleCoil},
        {gettext("06 Write Single Register"), ModbusWriteSingleRegister},
        {gettext("15 Write Multiple Coils"), ModbusWriteMultipleCoils},
        {gettext("16 Write Multiple Registers"), ModbusWriteMultipleRegisters}};
    modbus_slave_function_map = {
        {gettext("01 Read Coils (0x)"), ModbusReadCoils},
        {gettext("02 Read Descrete Inputs (1x)"), ModbusReadDescreteInputs},
        {gettext("03 Read Holding Registers (4x)"), ModbusReadHoldingRegisters},
        {gettext("04 Read Input Registers (3x)"), ModbusReadInputRegisters}};
    modbus_error_code_map = {
        {ModbusErrorCode_Timeout, gettext("Timeout Error")},
        {ModbusErrorCode_Illegal_Function, gettext("Illegal Function")},
        {ModbusErrorCode_Illegal_Data_Address, gettext("Illegal Data Address")},
        {ModbusErrorCode_Illegal_Data_Value, gettext("Illegal Data Value")},
        {ModbusErrorCode_Slave_Device_Failure, gettext("Slave Device Failure")},
        {ModbusErrorCode_Acknowledge, gettext("Acknowledge")},
        {ModbusErrorCode_Slave_Device_Busy, gettext("Slave Device Busy")},
        {ModbusErrorCode_Negative_Acknowledgment, gettext("Negative Acknowledgment")},
        {ModbusErrorCode_Memory_Parity_Error, gettext("Memory Parity Error")},
        {ModbusErrorCode_Gateway_Path_Unavailable, gettext("Gateway Path Unavailable")},
        {ModbusErrorCode_Gateway_Target_Device_Failed_To_Respond, gettext("Gateway Target Device Failed To Respond")}};
    modbus_error_code_comment_map = {
        {ModbusErrorCode_Timeout, gettext("The slave did not reply within the specified time.")},
        {ModbusErrorCode_Illegal_Function, gettext("The function code received in the request is not an authorized action for the slave. The slave may be in the wrong state to process a specific request.")},
        {ModbusErrorCode_Illegal_Data_Address, gettext("The data address received by the slave is not an authorized address for the slave.")},
        {ModbusErrorCode_Illegal_Data_Value, gettext("The value in the request data field is not an authorized value for the slave.")},
        {ModbusErrorCode_Slave_Device_Failure, gettext("The slave fails to perform a requested action because of an unrecoverable error.")},
        {ModbusErrorCode_Acknowledge, gettext("The slave accepts the request but needs a long time to process it.")},
        {ModbusErrorCode_Slave_Device_Busy, gettext("The slave is busy processing another command. The master must send the request once the slave is available.")},
        {ModbusErrorCode_Negative_Acknowledgment, gettext("The slave cannot perform the programming request sent by the master.")},
        {ModbusErrorCode_Memory_Parity_Error, gettext("The slave detects a parity error in the memory when attempting to read extended memory.")},
        {ModbusErrorCode_Gateway_Path_Unavailable, gettext("The gateway is overloaded or not correctly configured.")},
        {ModbusErrorCode_Gateway_Target_Device_Failed_To_Respond, gettext("The slave is not present on the network.")}};
    m_write_format_map = {
        {gettext("Signed"), Format_Signed},
        {gettext("Unsigned"), Format_Unsigned},
        {gettext("Long ABCD"), Format_32_Bit_Signed_Big_Endian},
        {gettext("Long CDAB"), Format_32_Bit_Signed_Little_Endian},
        {gettext("Long BADC"), Format_32_Bit_Signed_Big_Endian_Byte_Swap},
        {gettext("Long DCBA"), Format_32_Bit_Signed_Little_Endian_Byte_Swap},
        {gettext("Float ABCD"), Format_32_Bit_Float_Big_Endian},
        {gettext("Float CDAB"), Format_32_Bit_Float_Little_Endian},
        {gettext("Float BADC"), Format_32_Bit_Float_Big_Endian_Byte_Swap},
        {gettext("Float DCBA"), Format_32_Bit_Float_Little_Endian_Byte_Swap},
        {gettext("Double ABCDEFGH"), Format_64_Bit_Float_Big_Endian},
        {gettext("Double GHEFCDAB"), Format_64_Bit_Float_Little_Endian},
        {gettext("Double BADCFEHG"), Format_64_Bit_Float_Big_Endian_Byte_Swap},
        {gettext("Double HGFEDCBA"), Format_64_Bit_Float_Little_Endian_Byte_Swap}
    };
    if (m_identifier == ModbusMaster)
    {
        m_scan_timer_id = SDL_AddTimer(1, Scan_timer_callback, this);
        m_send_timer_id = SDL_AddTimer(1, Send_timer_callback, this);
    }
}

ModbusWindow::~ModbusWindow()
{
    SDL_RemoveTimer(m_scan_timer_id);
    SDL_RemoveTimer(m_send_timer_id);
    SDL_RemoveTimer(m_recv_timer_id);
    delete m_myIODevice;
    std::for_each(m_registers_table_datas.begin(), m_registers_table_datas.end(), [](RegistersTableData *data)
                  { delete data; });
    std::for_each(m_cycle_list.begin(), m_cycle_list.end(), [](ModbusPacket *data)
                  { delete data; });
    std::for_each(m_manual_list.begin(), m_manual_list.end(), [](ModbusPacket *data)
                  { delete data; });
}

void ModbusWindow::render()
{
    ImGui::SetNextWindowSize(ImVec2(400, 700), ImGuiCond_FirstUseEver);
    if (ImGui::Begin(m_window_name, &m_visible, ImGuiWindowFlags_MenuBar))
    {
        render_menu_bar();
        render_registers_tables();
    }
    ImGui::End();
    if (m_add_registers_dialog_visible)
    {
        render_add_registers_dialog();
    }
    if (m_modify_registers_dialog_visible)
    {
        render_modify_registers_dialog();
    }
    if (m_communication_traffic_dialog_visible)
    {
        render_communication_traffic_dialog();
    }
    if (m_error_counter_dialog_visible)
    {
        render_error_counter_dialog();
    }
    if (m_timeout_setting_dialog_visible)
    {
        render_timeout_setting_dialog();
    }
    if (m_modbus_function_05_dialog_visible)
    {
        render_modbus_function_05_dialog();
    }
    if (m_modbus_function_06_dialog_visible)
    {
        render_modbus_function_06_dialog();
    }
    if (m_modbus_function_15_dialog_visible)
    {
        render_modbus_function_15_dialog();
    }
    if (m_modbus_function_16_dialog_visible)
    {
        render_modbus_function_16_dialog();
    }
}

void ModbusWindow::render_menu_bar()
{
    if (ImGui::BeginMenuBar())
    {
        if (m_identifier == ModbusMaster)
        {
            render_master_menu();
        }
        else if (m_identifier == ModbusSlave)
        {
            render_slave_menu();
        }
        ImGui::EndMenuBar();
    }
}

void ModbusWindow::render_master_menu()
{
    if (ImGui::BeginMenu(gettext("Tools")))
    {
        ImGui::MenuItem(gettext("Add registers"), nullptr, &m_add_registers_dialog_visible);
        ImGui::MenuItem(gettext("Modify registers"), nullptr, &m_modify_registers_dialog_visible);
        ImGui::MenuItem(gettext("Communication traffic"), nullptr, &m_communication_traffic_dialog_visible);
        ImGui::MenuItem(gettext("Error counter"), nullptr, &m_error_counter_dialog_visible);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(gettext("Settings")))
    {
        ImGui::MenuItem(gettext("Timeout Setting"), nullptr, &m_timeout_setting_dialog_visible);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu(gettext("Functions")))
    {
        ImGui::MenuItem(gettext("05 : Write Single Coil"), nullptr, &m_modbus_function_05_dialog_visible);
        ImGui::MenuItem(gettext("06 : Write Single Register"), nullptr, &m_modbus_function_06_dialog_visible);
        ImGui::MenuItem(gettext("15 : Write Multiple Coils"), nullptr, &m_modbus_function_15_dialog_visible);
        ImGui::MenuItem(gettext("16 : Write Multiple Registers"), nullptr, &m_modbus_function_16_dialog_visible);
        ImGui::EndMenu();
    }
}

void ModbusWindow::render_slave_menu()
{
    if (ImGui::BeginMenu(gettext("Tools")))
    {
        ImGui::MenuItem(gettext("Add registers"), nullptr, &m_add_registers_dialog_visible);
        ImGui::MenuItem(gettext("Modify registers"), nullptr, &m_modify_registers_dialog_visible);
        ImGui::MenuItem(gettext("Communication traffic"), nullptr, &m_communication_traffic_dialog_visible);
        ImGui::EndMenu();
    }
}

void ModbusWindow::render_registers_tables()
{
    char value_str[32];
    for (auto &x : m_registers_table_datas)
    {

        if (ImGui::CollapsingHeader(x->table_title, &x->table_visible))
        {
            ImGui::Text("%s", x->info);
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s", x->msg);
            if (ImGui::BeginTable(x->table_title, 3, ImGuiTableFlags_Borders | ImGuiTableFlags_Resizable | ImGuiTableFlags_ContextMenuInBody))
            {
                ImGui::TableSetupColumn(gettext("Address"));
                ImGui::TableSetupColumn(gettext("Alias"));
                ImGui::TableSetupColumn(gettext("Value"));
                ImGui::TableHeadersRow();
                for (int i = 0; i < x->reg_quantity; ++i)
                {
                    ImGui::TableNextRow();
                    ImGui::TableNextColumn();
                    ImGui::Text("%u", x->reg_start + i);
                    ImGui::TableNextColumn();
                    ImGui::PushID(i);
                    x->reg_alias_selected[i] = ImGui::SelectableInput("##i", x->reg_alias_selected[i], ImGuiSelectableFlags_None, x->reg_alias[i], REGISTER_ALIAS_MAX_LEN);
                    ImGui::PopID();
                    ImGui::TableNextColumn();
                    get_value_by_format(x->cell_formats[i], &x->reg_values[i], value_str, sizeof(value_str));
                    ImGui::PushID(i + x->reg_quantity);
                    if (ImGui::SelectableInput("##i", x->reg_values_selected[i], ImGuiSelectableFlags_None, value_str, sizeof(value_str)))
                    {
                        if (m_identifier == ModbusSlave || x->function == ModbusWriteMultipleCoils || x->function == ModbusWriteMultipleRegisters || x->function == ModbusWriteSingleCoil || x->function == ModbusWriteSingleRegister)
                        {
                            set_value_by_format(x->cell_formats[i], &x->reg_values[i], value_str);
                        }
                        else if (m_identifier == ModbusMaster && (x->function == ModbusReadCoils || x->function == ModbusReadHoldingRegisters))
                        {
                            write_master_register_value(x->cell_formats[i], value_str, x, i);
                        }
                    }
                    ImGui::PopID();
                    ImGui::PushID(i + 2 * x->reg_quantity);
                    if (ImGui::BeginPopupContextItem())
                    {
                        if (ImGui::BeginMenu(gettext("Format")))
                        {
                            bool disable_format = x->function == ModbusReadCoils || x->function == ModbusReadDescreteInputs || x->function == ModbusWriteSingleCoil || x->function == ModbusWriteMultipleCoils;
                            if (disable_format)
                            {
                                ImGui::BeginDisabled(true);
                            }
                            if (ImGui::RadioButton(gettext("Signed"), (int *)&x->cell_formats[i], Format_Signed))
                            {
                                x->cell_formats[i] = Format_Signed;
                            }
                            if (ImGui::RadioButton(gettext("Unsigned"), (int *)&x->cell_formats[i], Format_Unsigned))
                            {
                                x->cell_formats[i] = Format_Unsigned;
                            }
                            if (ImGui::RadioButton(gettext("Hex"), (int *)&x->cell_formats[i], Format_Hex))
                            {
                                x->cell_formats[i] = Format_Hex;
                            }
                            if (ImGui::RadioButton(gettext("ASCII - Hex"), (int *)&x->cell_formats[i], Format_Ascii_Hex))
                            {
                                x->cell_formats[i] = Format_Ascii_Hex;
                            }
                            if (ImGui::RadioButton(gettext("Binary"), (int *)&x->cell_formats[i], Format_Binary))
                            {
                                x->cell_formats[i] = Format_Binary;
                            }
                            bool format_changed = false;
                            if (ImGui::BeginMenu(gettext("32-bit Signed")))
                            {
                                if (ImGui::RadioButton(gettext("Big Endian"), (int *)&x->cell_formats[i], Format_32_Bit_Signed_Big_Endian))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Little Endian"), (int *)&x->cell_formats[i], Format_32_Bit_Signed_Little_Endian))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Big Endian Byte Swap"), (int *)&x->cell_formats[i], Format_32_Bit_Signed_Big_Endian_Byte_Swap))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Little Endian Byte Swap"), (int *)&x->cell_formats[i], Format_32_Bit_Signed_Little_Endian_Byte_Swap))
                                {
                                    format_changed = true;
                                }
                                ImGui::EndMenu();
                            }
                            if (ImGui::BeginMenu(gettext("32-bit Unsigned")))
                            {
                                if (ImGui::RadioButton(gettext("Big Endian"), (int *)&x->cell_formats[i], Format_32_Bit_Unsigned_Big_Endian))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Little Endian"), (int *)&x->cell_formats[i], Format_32_Bit_Unsigned_Little_Endian))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Big Endian Byte Swap"), (int *)&x->cell_formats[i], Format_32_Bit_Unsigned_Big_Endian_Byte_Swap))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Little Endian Byte Swap"), (int *)&x->cell_formats[i], Format_32_Bit_Unsigned_Little_Endian_Byte_Swap))
                                {
                                    format_changed = true;
                                }
                                ImGui::EndMenu();
                            }
                            if (format_changed)
                            {
                                if (i < x->reg_quantity - 1)
                                {
                                    x->cell_formats[i + 1] = Format_None;
                                }
                            }
                            format_changed = false;
                            if (ImGui::BeginMenu(gettext("64-bit Signed")))
                            {
                                if (ImGui::RadioButton(gettext("Big Endian"), (int *)&x->cell_formats[i], Format_64_Bit_Signed_Big_Endian))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Little Endian"), (int *)&x->cell_formats[i], Format_64_Bit_Signed_Little_Endian))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Big Endian Byte Swap"), (int *)&x->cell_formats[i], Format_64_Bit_Signed_Big_Endian_Byte_Swap))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Little Endian Byte Swap"), (int *)&x->cell_formats[i], Format_64_Bit_Signed_Little_Endian_Byte_Swap))
                                {
                                    format_changed = true;
                                }
                                ImGui::EndMenu();
                            }
                            if (ImGui::BeginMenu(gettext("64-bit Unsigned")))
                            {
                                if (ImGui::RadioButton(gettext("Big Endian"), (int *)&x->cell_formats[i], Format_64_Bit_Unsigned_Big_Endian))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Little Endian"), (int *)&x->cell_formats[i], Format_64_Bit_Unsigned_Little_Endian))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Big Endian Byte Swap"), (int *)&x->cell_formats[i], Format_64_Bit_Unsigned_Big_Endian_Byte_Swap))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Little Endian Byte Swap"), (int *)&x->cell_formats[i], Format_64_Bit_Unsigned_Little_Endian_Byte_Swap))
                                {
                                    format_changed = true;
                                }
                                ImGui::EndMenu();
                            }
                            if (format_changed)
                            {
                                if (i < x->reg_quantity - 3)
                                {
                                    x->cell_formats[i + 1] = Format_None;
                                    x->cell_formats[i + 2] = Format_None;
                                    x->cell_formats[i + 3] = Format_None;
                                }
                            }
                            ImGui::Separator();
                            format_changed = false;
                            if (ImGui::BeginMenu(gettext("32-bit Float")))
                            {
                                if (ImGui::RadioButton(gettext("Big Endian"), (int *)&x->cell_formats[i], Format_32_Bit_Float_Big_Endian))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Little Endian"), (int *)&x->cell_formats[i], Format_32_Bit_Float_Little_Endian))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Big Endian Byte Swap"), (int *)&x->cell_formats[i], Format_32_Bit_Float_Big_Endian_Byte_Swap))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Little Endian Byte Swap"), (int *)&x->cell_formats[i], Format_32_Bit_Float_Little_Endian_Byte_Swap))
                                {
                                    format_changed = true;
                                }
                                ImGui::EndMenu();
                            }
                            if (format_changed)
                            {
                                if (i < x->reg_quantity - 1)
                                {
                                    x->cell_formats[i + 1] = Format_None;
                                }
                            }
                            format_changed = false;
                            if (ImGui::BeginMenu(gettext("64-bit Float")))
                            {
                                if (ImGui::RadioButton(gettext("Big Endian"), (int *)&x->cell_formats[i], Format_64_Bit_Float_Big_Endian))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Little Endian"), (int *)&x->cell_formats[i], Format_64_Bit_Float_Little_Endian))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Big Endian Byte Swap"), (int *)&x->cell_formats[i], Format_64_Bit_Float_Big_Endian_Byte_Swap))
                                {
                                    format_changed = true;
                                }
                                if (ImGui::RadioButton(gettext("Little Endian Byte Swap"), (int *)&x->cell_formats[i], Format_64_Bit_Float_Little_Endian_Byte_Swap))
                                {
                                    format_changed = true;
                                }
                                ImGui::EndMenu();
                            }
                            if (format_changed)
                            {
                                if (i < x->reg_quantity - 3)
                                {
                                    x->cell_formats[i + 1] = Format_None;
                                    x->cell_formats[i + 2] = Format_None;
                                    x->cell_formats[i + 3] = Format_None;
                                }
                            }
                            if (disable_format)
                            {
                                ImGui::EndDisabled();
                            }
                            ImGui::EndMenu();
                        }
                        ImGui::EndPopup();
                    }
                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
        }
    }
    for (auto iter = m_registers_table_datas.begin(); iter != m_registers_table_datas.end();)
    {
        if ((*iter)->table_visible)
        {
            ++iter;
        }
        else
        {
            delete *iter;
            iter = m_registers_table_datas.erase(iter);
        }
    }
}

void ModbusWindow::get_value_by_format(CellFormat format, uint16_t *value_ptr, char *value_str, int max_len)
{
    switch (format)
    {
    case Format_None:
    {
        snprintf(value_str, max_len, "--");
        break;
    }
    case Format_Coil:
    {
        snprintf(value_str, max_len, "%u", *value_ptr ? 1 : 0);
        break;
    }
    case Format_Signed:
    {
        snprintf(value_str, max_len, "%d", int16_t(*value_ptr));
        break;
    }
    case Format_Unsigned:
    {
        snprintf(value_str, max_len, "%u", *value_ptr);
        break;
    }
    case Format_Hex:
    {
        snprintf(value_str, max_len, "0x%X", *value_ptr);
        break;
    }
    case Format_Ascii_Hex:
    {
        snprintf(value_str, 11, "0x%X(%s)", *value_ptr, (char *)value_ptr);
        break;
    }
    case Format_Binary:
    {
        char bin_str[33];
        SDL_itoa(*value_ptr, bin_str, 2);
        snprintf(value_str, max_len, "%s", bin_str);
        break;
    }
    case Format_32_Bit_Signed_Big_Endian:
    {
        int32_t value = myFromBigEndian<int32_t>(value_ptr);
        snprintf(value_str, max_len, "%d", value);
        break;
    }
    case Format_32_Bit_Signed_Little_Endian:
    {
        int32_t value = myFromLittleEndian<int32_t>(value_ptr);
        snprintf(value_str, max_len, "%d", value);
        break;
    }
    case Format_32_Bit_Signed_Big_Endian_Byte_Swap:
    {
        int32_t value = myFromBigEndianByteSwap<int32_t>(value_ptr);
        snprintf(value_str, max_len, "%d", value);
        break;
    }
    case Format_32_Bit_Signed_Little_Endian_Byte_Swap:
    {
        int32_t value = myFromLittleEndianByteSwap<int32_t>(value_ptr);
        snprintf(value_str, max_len, "%d", value);
        break;
    }
    case Format_32_Bit_Unsigned_Big_Endian:
    {
        uint32_t value = myFromBigEndian<uint32_t>(value_ptr);
        snprintf(value_str, max_len, "%u", value);
        break;
    }
    case Format_32_Bit_Unsigned_Little_Endian:
    {
        uint32_t value = myFromLittleEndian<uint32_t>(value_ptr);
        snprintf(value_str, max_len, "%u", value);
        break;
    }
    case Format_32_Bit_Unsigned_Big_Endian_Byte_Swap:
    {
        uint32_t value = myFromBigEndianByteSwap<uint32_t>(value_ptr);
        snprintf(value_str, max_len, "%u", value);
        break;
    }
    case Format_32_Bit_Unsigned_Little_Endian_Byte_Swap:
    {
        uint32_t value = myFromLittleEndianByteSwap<uint32_t>(value_ptr);
        snprintf(value_str, max_len, "%u", value);
        break;
    }
    case Format_64_Bit_Signed_Big_Endian:
    {
        int64_t value = myFromBigEndian<int64_t>(value_ptr);
        snprintf(value_str, max_len, "%ld", value);
        break;
    }
    case Format_64_Bit_Signed_Little_Endian:
    {
        int64_t value = myFromLittleEndian<int64_t>(value_ptr);
        snprintf(value_str, max_len, "%ld", value);
        break;
    }
    case Format_64_Bit_Signed_Big_Endian_Byte_Swap:
    {
        int64_t value = myFromBigEndianByteSwap<int64_t>(value_ptr);
        snprintf(value_str, max_len, "%ld", value);
        break;
    }
    case Format_64_Bit_Signed_Little_Endian_Byte_Swap:
    {
        int64_t value = myFromLittleEndianByteSwap<int64_t>(value_ptr);
        snprintf(value_str, max_len, "%ld", value);
        break;
    }
    case Format_64_Bit_Unsigned_Big_Endian:
    {
        uint64_t value = myFromBigEndian<uint64_t>(value_ptr);
        snprintf(value_str, max_len, "%lu", value);
        break;
    }
    case Format_64_Bit_Unsigned_Little_Endian:
    {
        uint64_t value = myFromLittleEndian<uint64_t>(value_ptr);
        snprintf(value_str, max_len, "%lu", value);
        break;
    }
    case Format_64_Bit_Unsigned_Big_Endian_Byte_Swap:
    {
        uint64_t value = myFromBigEndianByteSwap<uint64_t>(value_ptr);
        snprintf(value_str, max_len, "%lu", value);
        break;
    }
    case Format_64_Bit_Unsigned_Little_Endian_Byte_Swap:
    {
        uint64_t value = myFromLittleEndianByteSwap<uint64_t>(value_ptr);
        snprintf(value_str, max_len, "%lu", value);
        break;
    }
    case Format_32_Bit_Float_Big_Endian:
    {
        float fval = myFromBigEndian<float>(value_ptr);
        snprintf(value_str, max_len, "%g", fval);
        break;
    }
    case Format_32_Bit_Float_Little_Endian:
    {
        float fval = myFromLittleEndian<float>(value_ptr);
        snprintf(value_str, max_len, "%g", fval);
        break;
    }
    case Format_32_Bit_Float_Big_Endian_Byte_Swap:
    {
        float fval = myFromBigEndianByteSwap<float>(value_ptr);
        snprintf(value_str, max_len, "%g", fval);
        break;
    }
    case Format_32_Bit_Float_Little_Endian_Byte_Swap:
    {
        float fval = myFromLittleEndianByteSwap<float>(value_ptr);
        snprintf(value_str, max_len, "%g", fval);
        break;
    }
    case Format_64_Bit_Float_Big_Endian:
    {
        double dval = myFromBigEndian<double>(value_ptr);
        snprintf(value_str, max_len, "%g", dval);
        break;
    }
    case Format_64_Bit_Float_Little_Endian:
    {
        double dval = myFromLittleEndian<double>(value_ptr);
        snprintf(value_str, max_len, "%g", dval);
        break;
    }
    case Format_64_Bit_Float_Big_Endian_Byte_Swap:
    {
        double dval = myFromBigEndianByteSwap<double>(value_ptr);
        snprintf(value_str, max_len, "%g", dval);
        break;
    }
    case Format_64_Bit_Float_Little_Endian_Byte_Swap:
    {
        double dval = myFromLittleEndianByteSwap<double>(value_ptr);
        snprintf(value_str, max_len, "%g", dval);
        break;
    }
    default:
    {
        snprintf(value_str, max_len, "Unknown Format");
        break;
    }
    }
}

void ModbusWindow::set_value_by_format(CellFormat format, uint16_t *value_ptr, const char *value_str)
{
    switch (format)
    {
    case Format_Coil:
    {
        *value_ptr = (*value_ptr != '0');
        break;
    }
    case Format_Signed:
    case Format_Unsigned:
    {
        try
        {
            *value_ptr = std::stoi(value_str);
        }
        catch (std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_Hex:
    {
        try
        {
            *value_ptr = std::stoi(value_str, nullptr, 16);
        }
        catch (std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_Ascii_Hex:
    {
        break;
    }
    case Format_Binary:
    {
        try
        {
            *value_ptr = std::stoi(value_str, nullptr, 2);
        }
        catch (std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Unsigned_Big_Endian:
    case Format_32_Bit_Signed_Big_Endian:
    {
        try
        {
            int32_t value = std::stoi(value_str);
            myToBigEndianByteSwap(value, value_ptr);
        }
        catch (std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Unsigned_Little_Endian:
    case Format_32_Bit_Signed_Little_Endian:
    {
        try
        {
            int32_t value = std::stoi(value_str);
            myToLittleEndianByteSwap(value, value_ptr);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Unsigned_Big_Endian_Byte_Swap:
    case Format_32_Bit_Signed_Big_Endian_Byte_Swap:
    {
        try
        {
            int32_t value = std::stoi(value_str);
            myToBigEndian(value, value_ptr);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Unsigned_Little_Endian_Byte_Swap:
    case Format_32_Bit_Signed_Little_Endian_Byte_Swap:
    {
        try
        {
            int32_t value = std::stoi(value_str);
            myToLittleEndian(value, value_ptr);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Float_Big_Endian:
    {
        try
        {
            float fval = std::stof(value_str);
            myToBigEndianByteSwap(fval, value_ptr);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Float_Little_Endian:
    {
        try
        {
            float fval = std::stof(value_str);
            myToLittleEndianByteSwap(fval, value_ptr);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Float_Big_Endian_Byte_Swap:
    {
        try
        {
            float fval = std::stof(value_str);
            myToBigEndian(fval, value_ptr);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Float_Little_Endian_Byte_Swap:
    {
        try
        {
            float fval = std::stof(value_str);
            myToLittleEndian(fval, value_ptr);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_64_Bit_Unsigned_Big_Endian:
    case Format_64_Bit_Signed_Big_Endian:
    {
        try
        {
            int64_t value = std::stoll(value_str);
            myToBigEndianByteSwap(value, value_ptr);
        }
        catch (std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_64_Bit_Unsigned_Little_Endian:
    case Format_64_Bit_Signed_Little_Endian:
    {
        try
        {
            int64_t value = std::stoll(value_str);
            myToLittleEndianByteSwap(value, value_ptr);
        }
        catch (std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_64_Bit_Unsigned_Big_Endian_Byte_Swap:
    case Format_64_Bit_Signed_Big_Endian_Byte_Swap:
    {
        try
        {
            int64_t value = std::stoll(value_str);
            myToBigEndian(value, value_ptr);
        }
        catch (std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_64_Bit_Float_Big_Endian:
    {
        try
        {
            double value = std::stod(value_str);
            myToBigEndianByteSwap(value, value_ptr);
        }
        catch (std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_64_Bit_Float_Little_Endian:
    {
        try
        {
            double value = std::stod(value_str);
            myToLittleEndianByteSwap(value, value_ptr);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_64_Bit_Float_Big_Endian_Byte_Swap:
    {
        try
        {
            double value = std::stod(value_str);
            myToBigEndian(value, value_ptr);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_64_Bit_Float_Little_Endian_Byte_Swap:
    {
        try
        {
            double value = std::stod(value_str);
            myToLittleEndian(value, value_ptr);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    default:
    {
        break;
    }
    }
}

void ModbusWindow::render_add_registers_dialog()
{
    char window_name[160];
    snprintf(window_name, sizeof(window_name), "%s(%s)", gettext("Add Registers"), m_window_name);
    std::unordered_map<const char *, ModbusFunctions> &function_map = (m_identifier == ModbusMaster) ? modbus_function_map : modbus_slave_function_map;
    if (ImGui::Begin(window_name, &m_add_registers_dialog_visible))
    {
        ImGui::InputInt("ID", &m_add_reg_def_data.id, 1, 10);
        render_combo_box(m_add_reg_def_data.function_combo_box_data, gettext("Function"), getKeysOfMap(function_map).data(), 0, function_map.size() - 1);
        ModbusFunctions function = function_map[m_add_reg_def_data.function_combo_box_data.text];
        ImGui::InputInt(gettext("Address"), &m_add_reg_def_data.reg_addr, 10, 1000);
        ImGui::InputInt(gettext("Quantity"), &m_add_reg_def_data.quantity, 10, 100);
        if (m_identifier == ModbusMaster)
        {
            ImGui::InputInt(gettext("Scan Rate"), &m_add_reg_def_data.scan_rate, 100, 1000);
            ImGui::Separator();
            if (m_add_reg_def_data.frame_info.id != m_add_reg_def_data.id || m_add_reg_def_data.frame_info.function != function || m_add_reg_def_data.frame_info.reg_addr != m_add_reg_def_data.reg_addr || m_add_reg_def_data.frame_info.quantity != m_add_reg_def_data.quantity)
            {
                m_add_reg_def_data.frame_info.id = m_add_reg_def_data.id;
                m_add_reg_def_data.frame_info.function = function;
                m_add_reg_def_data.frame_info.reg_addr = m_add_reg_def_data.reg_addr;
                m_add_reg_def_data.frame_info.quantity = m_add_reg_def_data.quantity;
                m_add_reg_def_data.packet_size = m_modbus->masterFrame2Pack(m_add_reg_def_data.frame_info, m_add_reg_def_data.packet);
                toHexString((const uint8_t *)m_add_reg_def_data.packet, m_add_reg_def_data.packet_size, m_add_reg_def_data.hex_str);
            }
            ImGui::Text("%s", m_add_reg_def_data.hex_str);
            ImGui::Separator();
        }
        if (ImGui::Button(gettext("OK"), ImVec2(ImGui::GetWindowSize().x / 2 - 10, 35)))
        {
            RegistersTableData *regs_table_data = new RegistersTableData(m_add_reg_def_data.id, m_add_reg_def_data.reg_addr, m_add_reg_def_data.reg_addr + m_add_reg_def_data.quantity - 1, function, m_add_reg_def_data.scan_rate, m_identifier);
            if (m_identifier == ModbusMaster)
            {
                memcpy(regs_table_data->packet, m_add_reg_def_data.packet, m_add_reg_def_data.packet_size);
                regs_table_data->packet_size = m_add_reg_def_data.packet_size;
            }
            snprintf(regs_table_data->table_title, sizeof(regs_table_data->table_title), "ID:%d - Registers:(%d,%d)", regs_table_data->id, regs_table_data->reg_start, regs_table_data->reg_end);
            m_registers_table_datas.push_back(regs_table_data);
            m_add_registers_dialog_visible = false;
        }
        ImGui::SameLine(0.0f, 10);
        if (ImGui::Button(gettext("Cancel"), ImVec2(ImGui::GetWindowSize().x / 2 - 10, 35)))
        {
            m_add_registers_dialog_visible = false;
        }
    }
    ImGui::End();
}

void ModbusWindow::render_modify_registers_dialog()
{
    if (m_registers_table_datas.empty())
    {
        m_modify_registers_dialog_visible = false;
        return;
    }
    if (ImGui::Begin(gettext("Modify Registers"), &m_modify_registers_dialog_visible))
    {
        std::vector<const char *> reg_table_names;
        reg_table_names.reserve(m_registers_table_datas.size());
        std::for_each(m_registers_table_datas.begin(), m_registers_table_datas.end(), [&](RegistersTableData *x)
                      { reg_table_names.push_back(x->table_title); });
        render_combo_box(m_modify_table_names_combo_box_data, gettext("Registers Tables"), reg_table_names.data(), 0, reg_table_names.size() - 1);
        auto reg_table_iter = std::find_if(m_registers_table_datas.begin(), m_registers_table_datas.end(), [this](RegistersTableData *x)
                                           { return strcmp(x->table_title, m_modify_table_names_combo_box_data.text) == 0; });
        std::unordered_map<const char *, ModbusFunctions> &function_map = (m_identifier == ModbusMaster) ? modbus_function_map : modbus_slave_function_map;
        std::vector<const char *> function_names = getKeysOfMap(function_map);
        if (m_last_selected_table_name == nullptr || strcmp(m_last_selected_table_name, m_modify_table_names_combo_box_data.text) != 0)
        {
            m_last_selected_table_name = m_modify_table_names_combo_box_data.text;
            m_modify_reg_def_data.id = (*reg_table_iter)->id;
            m_modify_reg_def_data.reg_addr = (*reg_table_iter)->reg_start;
            m_modify_reg_def_data.quantity = (*reg_table_iter)->reg_quantity;
            m_modify_reg_def_data.scan_rate = (*reg_table_iter)->scan_rate;
            m_modify_reg_def_data.packet_size = (*reg_table_iter)->packet_size;
            for (int i = 0; i < function_names.size(); ++i)
            {
                if (function_map[function_names[i]] == (*reg_table_iter)->function)
                {
                    m_modify_reg_def_data.function_combo_box_data.index = i;
                }
            }
        }

        ImGui::InputInt(gettext("ID"), &m_modify_reg_def_data.id, 1, 10);
        render_combo_box(m_modify_reg_def_data.function_combo_box_data, gettext("Function"), function_names.data(), 0, function_names.size() - 1);
        ImGui::InputInt(gettext("Address"), &m_modify_reg_def_data.reg_addr, 1, 10);
        ImGui::InputInt(gettext("Quantity"), &m_modify_reg_def_data.quantity, 1, 10);
        if (m_identifier == ModbusMaster)
        {
            ImGui::InputInt(gettext("Scan Rate"), &m_modify_reg_def_data.scan_rate, 10, 100);
            ImGui::Separator();
            if (m_modify_reg_def_data.frame_info.id != m_modify_reg_def_data.id || m_modify_reg_def_data.frame_info.function != function_map[m_modify_reg_def_data.function_combo_box_data.text] || m_modify_reg_def_data.frame_info.reg_addr != m_modify_reg_def_data.reg_addr || m_modify_reg_def_data.frame_info.quantity != m_modify_reg_def_data.quantity)
            {
                m_modify_reg_def_data.frame_info.id = m_modify_reg_def_data.id;
                m_modify_reg_def_data.frame_info.function = function_map[m_modify_reg_def_data.function_combo_box_data.text];
                m_modify_reg_def_data.frame_info.reg_addr = m_modify_reg_def_data.reg_addr;
                m_modify_reg_def_data.frame_info.quantity = m_modify_reg_def_data.quantity;
                m_modify_reg_def_data.packet_size = m_modbus->masterFrame2Pack(m_modify_reg_def_data.frame_info, m_modify_reg_def_data.packet);
                toHexString((const uint8_t *)m_modify_reg_def_data.packet, m_modify_reg_def_data.packet_size, m_modify_reg_def_data.hex_str);
            }
            ImGui::Text("%s", m_modify_reg_def_data.hex_str);
            ImGui::Separator();
        }
        if (ImGui::Button(gettext("OK"), ImVec2(ImGui::GetWindowSize().x - 10, 35)))
        {
            for (int i = 0; i < (*reg_table_iter)->reg_quantity; ++i)
            {
                delete[] (*reg_table_iter)->reg_alias[i];
            }
            (*reg_table_iter)->id = m_modify_reg_def_data.id;
            (*reg_table_iter)->function = function_map[m_modify_reg_def_data.function_combo_box_data.text];
            (*reg_table_iter)->reg_start = m_modify_reg_def_data.reg_addr;
            (*reg_table_iter)->reg_end = m_modify_reg_def_data.reg_addr + m_modify_reg_def_data.quantity - 1;
            (*reg_table_iter)->reg_quantity = m_modify_reg_def_data.quantity;
            (*reg_table_iter)->scan_rate = m_modify_reg_def_data.scan_rate;
            memcpy((*reg_table_iter)->packet, m_modify_reg_def_data.packet, m_modify_reg_def_data.packet_size);
            (*reg_table_iter)->packet_size = m_modify_reg_def_data.packet_size;
            snprintf((*reg_table_iter)->table_title, sizeof((*reg_table_iter)->table_title), "ID:%d - Registers:(%d,%d)", (*reg_table_iter)->id, (*reg_table_iter)->reg_start, (*reg_table_iter)->reg_end);
            (*reg_table_iter)->update_info();
            (*reg_table_iter)->modify_registers();
            m_modify_registers_dialog_visible = false;
        }
    }
    ImGui::End();
}

void ModbusWindow::render_communication_traffic_dialog()
{
    if (ImGui::Begin(gettext("Communication Traffic"), &m_communication_traffic_dialog_visible))
    {
        if (ImGui::Button(m_communication_traffic_window_data.stopped ? gettext("Start") : gettext("Stop")))
        {
            m_communication_traffic_window_data.stopped = !m_communication_traffic_window_data.stopped;
        }
        ImGui::SameLine();
        if (ImGui::Button(gettext("Clear")))
        {
            m_communication_traffic_window_data.communication_traffic_text.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button(gettext("Save")))
        {
            char file_name[] = "./communication_traffic.txt";
            std::fstream file(file_name, std::ios::out);
            file << m_communication_traffic_window_data.communication_traffic_text;
            file.close();
        }
        ImGui::SameLine();
        if (ImGui::Button(gettext("Copy")))
        {
            ImGui::SetClipboardText(m_communication_traffic_window_data.communication_traffic_text.c_str());
        }
        ImGui::SameLine();
        ImGui::Checkbox(gettext("Stop On Error"), &m_communication_traffic_window_data.stop_on_error);
        ImGui::SameLine();
        ImGui::Checkbox(gettext("Timestamp"), &m_communication_traffic_window_data.timestamp);
        ImGui::Separator();
        ImGui::InputTextMultiline("##communication_traffic_text", m_communication_traffic_window_data.communication_traffic_text.data(), m_communication_traffic_window_data.communication_traffic_text.size(), ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight() - 50 - ImGui::GetWindowContentRegionMin().y), ImGuiInputTextFlags_ReadOnly);
    }
    ImGui::End();
    if (!m_communication_traffic_dialog_visible)
    {
        m_communication_traffic_window_data.communication_traffic_text.clear();
        m_communication_traffic_window_data.stopped = false;
    }
}

void ModbusWindow::render_error_counter_dialog()
{
    if (ImGui::Begin(gettext("Error Counter"), &m_error_counter_dialog_visible))
    {
        for (auto iter = m_error_count_map.begin(); iter != m_error_count_map.end(); ++iter)
        {
            ImGui::Text("\t%d\t\t%s", iter->second, modbus_error_code_map[iter->first]);
            ImGui::SetItemTooltip("%s", modbus_error_code_comment_map[iter->first]);
            ImGui::Separator();
        }
    }
    ImGui::End();
}

void ModbusWindow::render_timeout_setting_dialog()
{
    if (ImGui::Begin(gettext("Timeout Setting"), &m_timeout_setting_dialog_visible))
    {
        if (ImGui::InputInt(gettext("Timeout(ms)"), &m_tmp_recv_timeout_ms, 10, 1000, ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::Button(gettext("OK"), ImVec2(ImGui::GetWindowWidth() - 10, 35)))
        {
            if (m_tmp_recv_timeout_ms > 200)
            {
                m_recv_timeout_ms = m_tmp_recv_timeout_ms;
            }
            m_timeout_setting_dialog_visible = false;
        }
    }
    ImGui::End();
}

void ModbusWindow::render_modbus_function_05_dialog()
{
    if (ImGui::Begin(gettext("05:Write Single Coil"), &m_modbus_function_05_dialog_visible))
    {
        ImGui::DragInt(gettext("Slave ID"), &m_function_05_data.slave_id, 1, 1, 255);
        ImGui::InputInt(gettext("Address"), &m_function_05_data.address);
        ImGui::Separator();
        ImGui::Indent();
        ImGui::RadioButton(gettext("On"), &m_function_05_data.value, 1);
        ImGui::SameLine();
        ImGui::RadioButton(gettext("Off"), &m_function_05_data.value, 0);
        ImGui::Unindent();
        ImGui::Separator();
        ImGui::Checkbox(gettext("Close On Response OK"), &m_close_on_resp_ok);
        ImGui::Separator();
        ImGui::Indent();
        ImGui::RadioButton(gettext("Use 05 Function"), &m_function_05_data.function, 5);
        ImGui::SameLine();
        ImGui::RadioButton(gettext("Use 15 Function"), &m_function_05_data.function, 15);
        ImGui::Unindent();
        ImGui::Separator();
        if (m_write_frame_info.id != m_function_05_data.slave_id || m_write_frame_info.function != m_function_05_data.function || m_write_frame_info.reg_addr != m_function_05_data.address)
        {
            m_write_frame_info.id = m_function_05_data.slave_id;
            m_write_frame_info.function = m_function_05_data.function;
            m_write_frame_info.reg_addr = m_function_05_data.address;
            m_write_frame_info.quantity = 1;
            m_write_frame_info.reg_values[0] = m_function_05_data.value;
            m_function_05_data.packet_size = m_modbus->masterFrame2Pack(m_write_frame_info, m_function_05_data.packet);
            toHexString((const uint8_t *)m_function_05_data.packet, m_function_05_data.packet_size, m_function_05_data.hex_str);
        }
        ImGui::Text("%s", m_function_05_data.hex_str);
        ImGui::Separator();
        ImGui::Indent();
        if (ImGui::Button(gettext("Send"), ImVec2(ImGui::GetWindowWidth() / 3, 35)))
        {
            ModbusPacket *mdb_pack = new ModbusPacket;
            memcpy(mdb_pack->packet, m_function_05_data.packet, m_function_05_data.packet_size);
            mdb_pack->packet_size = m_function_05_data.packet_size;
            m_manual_list.push_back(mdb_pack);
        }
        ImGui::SameLine();
        if (ImGui::Button(gettext("Cancel"), ImVec2(ImGui::GetWindowWidth() / 3, 35)))
        {
            m_modbus_function_05_dialog_visible = false;
        }
        ImGui::Unindent();
    }
    ImGui::End();
}

void ModbusWindow::render_modbus_function_06_dialog()
{
    if(ImGui::Begin(gettext("06:Write Single Register"), &m_modbus_function_06_dialog_visible))
    {
        ImGui::DragInt(gettext("Slave ID"), &m_function_06_data.slave_id, 1, 1, 255);
        ImGui::InputInt(gettext("Address"), &m_function_06_data.address);
        ImGui::Separator();
        ImGui::InputInt(gettext("Value"), &m_function_06_data.value);
        ImGui::Separator();
        ImGui::Indent();
        ImGui::Checkbox(gettext("Close On Response OK"), &m_close_on_resp_ok);
        ImGui::Unindent();
        ImGui::Separator();
        ImGui::Indent();
        ImGui::RadioButton(gettext("Use 06 Function"), &m_function_06_data.function, 6);
        ImGui::SameLine();
        ImGui::RadioButton(gettext("Use 16 Function"), &m_function_06_data.function, 16);
        ImGui::Unindent();
        ImGui::Separator();
        if(m_write_frame_info.id != m_function_06_data.slave_id || m_write_frame_info.function != m_function_06_data.function || m_write_frame_info.reg_addr != m_function_06_data.address)
        {
            m_write_frame_info.id = m_function_06_data.slave_id;
            m_write_frame_info.function = m_function_06_data.function;
            m_write_frame_info.reg_addr = m_function_06_data.address;
            m_write_frame_info.quantity = 1;
            m_write_frame_info.reg_values[0] = m_function_06_data.value;
            m_function_06_data.packet_size = m_modbus->masterFrame2Pack(m_write_frame_info, m_function_06_data.packet);
            toHexString((const uint8_t *)m_function_06_data.packet, m_function_06_data.packet_size, m_function_06_data.hex_str);
        }
        ImGui::Text("%s", m_function_06_data.hex_str);
        ImGui::Separator();
        if(ImGui::Button(gettext("Send"), ImVec2(ImGui::GetWindowWidth() / 3, 35)))
        {
            ModbusPacket *mdb_pack = new ModbusPacket;
            memcpy(mdb_pack->packet, m_function_06_data.packet, m_function_06_data.packet_size);
            mdb_pack->packet_size = m_function_06_data.packet_size;
            m_manual_list.push_back(mdb_pack);
        }
        ImGui::SameLine();
        if(ImGui::Button(gettext("Cancel"), ImVec2(ImGui::GetWindowWidth() / 3, 35)))
        {
            m_modbus_function_06_dialog_visible = false;
        }
    }
    ImGui::End();
}

void ModbusWindow::render_modbus_function_15_dialog()
{
    if(ImGui::Begin(gettext("15:Write Multiple Coils"), &m_modbus_function_15_dialog_visible))
    {
        ImGui::BeginGroup();
        ImGui::BeginChild("left_panel", ImVec2(ImGui::GetWindowWidth() / 2, 0));
        ImGui::DragInt(gettext("Slave ID"), &m_function_15_data.slave_id, 1, 1, 255);
        ImGui::InputInt(gettext("Address"), &m_function_15_data.address);
        ImGui::DragInt(gettext("Quantity"), &m_function_15_data.quantity, 1, 1, MODBUS_FRAME_MAX_REGISTERS);
        ImGui::EndChild();
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginChild("center_panel", ImVec2(ImGui::GetWindowWidth() / 4, 0), ImGuiChildFlags_Border);
        char value_name[16];
        for(int i = 0; i < m_function_15_data.quantity; ++i)
        {
            snprintf(value_name, sizeof(value_name), "Coil %d", i + m_function_15_data.address);
            ImGui::Checkbox(value_name, &m_function_15_data.values[i].bool_value);
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::BeginChild("right_panel", ImVec2(ImGui::GetWindowWidth() / 4, 0));
        if(ImGui::Button(gettext("Reverse"), ImVec2(ImGui::GetWindowWidth(), 35)))
        {
            for(int i = 0; i < m_function_15_data.quantity; ++i)
            {
                m_function_15_data.values[i].bool_value = !m_function_15_data.values[i].bool_value;
            }
        }
        if(ImGui::Button(gettext("Send"), ImVec2(ImGui::GetWindowWidth(), 35)))
        {
            ModbusFrameInfo frame_info;
            frame_info.id = m_function_15_data.slave_id;
            frame_info.function = ModbusWriteMultipleCoils;
            frame_info.reg_addr = m_function_15_data.address;
            frame_info.quantity = m_function_15_data.quantity;
            uint8_t *coils = (uint8_t *)frame_info.reg_values;
            for(int i = 0; i < m_function_15_data.quantity; ++i)
            {
                int byte_index = i / 8;
                int bit_index = i % 8;
                setBit(coils[byte_index], bit_index, m_function_15_data.values[i].bool_value);
            }
            ModbusPacket *mdb_pack = new ModbusPacket;
            mdb_pack->packet_size = m_modbus->masterFrame2Pack(frame_info, mdb_pack->packet);
            m_manual_list.push_back(mdb_pack);
        }
        if(ImGui::Button(gettext("Cancel"), ImVec2(ImGui::GetWindowWidth(), 35)))
        {
            m_modbus_function_15_dialog_visible = false;
        }
        ImGui::EndChild();
        ImGui::EndGroup();
    }
    ImGui::End();
}

void ModbusWindow::render_modbus_function_16_dialog()
{
    if(ImGui::Begin(gettext("16:Write Multiple Registers"), &m_modbus_function_16_dialog_visible))
    {
        ImGui::BeginGroup();
        ImGui::BeginChild("left_panel", ImVec2(ImGui::GetWindowWidth() / 2, 0));
        ImGui::DragInt(gettext("Slave ID"), &m_function_16_data.slave_id, 1, 1, 255);
        ImGui::InputInt(gettext("Address"), &m_function_16_data.address);
        ImGui::DragInt(gettext("Quantity"), &m_function_16_data.quantity, 1, 1, MODBUS_FRAME_MAX_REGISTERS);
        render_combo_box(m_function_16_data.format_combo_box_data, gettext("Format"), getKeysOfMap(m_write_format_map).data(), 0, m_write_format_map.size() - 1);
        m_function_16_data.format = m_write_format_map[m_function_16_data.format_combo_box_data.text];
        ImGui::EndChild();
        ImGui::EndGroup();
        ImGui::SameLine();
        ImGui::BeginChild("center_panel", ImVec2(ImGui::GetWindowWidth() / 4, 0), ImGuiChildFlags_Border);
        m_write_frame_info.id = m_function_16_data.slave_id;
        m_write_frame_info.function = ModbusWriteMultipleRegisters;
        m_write_frame_info.reg_addr = m_function_16_data.address;
        switch(m_function_16_data.format)
        {
            case Format_Signed:
            {
                m_write_frame_info.quantity = m_function_16_data.quantity;
                for(int i = 0; i < m_function_16_data.quantity; ++i)
                {
                    snprintf(m_function_16_data.value_name[i], sizeof(m_function_16_data.value_name[i]), "%d", i + m_function_16_data.address);
                    ImGui::InputInt(m_function_16_data.value_name[i], &m_function_16_data.values[i].s32_value);
                    m_write_frame_info.reg_values[i] = m_function_16_data.values[i].s32_value;
                }
                break;
            }
            case Format_32_Bit_Signed_Big_Endian:
            {
                m_write_frame_info.quantity = m_function_16_data.quantity * 2;
                for(int i = 0; i < m_function_16_data.quantity; ++i)
                {
                    snprintf(m_function_16_data.value_name[i], sizeof(m_function_16_data.value_name[i]), "%d", i + m_function_16_data.address);
                    ImGui::InputInt(m_function_16_data.value_name[i], &m_function_16_data.values[i].s32_value);
                    myToBigEndianByteSwap(m_function_16_data.values[i].s32_value, &m_write_frame_info.reg_values[i * 2]);
                }
                break;
            }
            case Format_32_Bit_Signed_Little_Endian:
            {
                m_write_frame_info.quantity = m_function_16_data.quantity * 2;
                for(int i = 0; i < m_function_16_data.quantity; ++i)
                {
                    snprintf(m_function_16_data.value_name[i], sizeof(m_function_16_data.value_name[i]), "%d", i + m_function_16_data.address);
                    ImGui::InputInt(m_function_16_data.value_name[i], &m_function_16_data.values[i].s32_value);
                    myToLittleEndianByteSwap(m_function_16_data.values[i].s32_value, &m_write_frame_info.reg_values[i * 2]);
                }
                break;
            }
            case Format_32_Bit_Signed_Big_Endian_Byte_Swap:
            {
                m_write_frame_info.quantity = m_function_16_data.quantity * 2;
                for(int i = 0; i < m_function_16_data.quantity; ++i)
                {
                    snprintf(m_function_16_data.value_name[i], sizeof(m_function_16_data.value_name[i]), "%d", i + m_function_16_data.address);
                    ImGui::InputInt(m_function_16_data.value_name[i], &m_function_16_data.values[i].s32_value);
                    myToBigEndian(m_function_16_data.values[i].s32_value, &m_write_frame_info.reg_values[i * 2]);
                }
                break;
            }
            case Format_32_Bit_Signed_Little_Endian_Byte_Swap:
            {
                m_write_frame_info.quantity = m_function_16_data.quantity * 2;
                for(int i = 0; i < m_function_16_data.quantity; ++i)
                {
                    snprintf(m_function_16_data.value_name[i], sizeof(m_function_16_data.value_name[i]), "%d", i + m_function_16_data.address);
                    ImGui::InputInt(m_function_16_data.value_name[i], &m_function_16_data.values[i].s32_value);
                    myToLittleEndian(m_function_16_data.values[i].s32_value, &m_write_frame_info.reg_values[i * 2]);
                }
                break;
            }
            case Format_32_Bit_Float_Big_Endian:
            {
                m_write_frame_info.quantity = m_function_16_data.quantity * 2;
                for(int i = 0; i < m_function_16_data.quantity; ++i)
                {
                    snprintf(m_function_16_data.value_name[i], sizeof(m_function_16_data.value_name[i]), "%d", i + m_function_16_data.address);
                    ImGui::InputFloat(m_function_16_data.value_name[i], &m_function_16_data.values[i].f32_value);
                    myToBigEndianByteSwap(m_function_16_data.values[i].f32_value, &m_write_frame_info.reg_values[i * 2]);
                }
                break;
            }
            case Format_32_Bit_Float_Little_Endian:
            {
                m_write_frame_info.quantity = m_function_16_data.quantity * 2;
                for(int i = 0; i < m_function_16_data.quantity; ++i)
                {
                    snprintf(m_function_16_data.value_name[i], sizeof(m_function_16_data.value_name[i]), "%d", i + m_function_16_data.address);
                    ImGui::InputFloat(m_function_16_data.value_name[i], &m_function_16_data.values[i].f32_value);
                    myToLittleEndianByteSwap(m_function_16_data.values[i].f32_value, &m_write_frame_info.reg_values[i * 2]);
                }
                break;
            }
            case Format_32_Bit_Float_Big_Endian_Byte_Swap:
            {
                m_write_frame_info.quantity = m_function_16_data.quantity * 2;
                for(int i = 0; i < m_function_16_data.quantity; ++i)
                {
                    snprintf(m_function_16_data.value_name[i], sizeof(m_function_16_data.value_name[i]), "%d", i + m_function_16_data.address);
                    ImGui::InputFloat(m_function_16_data.value_name[i], &m_function_16_data.values[i].f32_value);
                    myToBigEndian(m_function_16_data.values[i].f32_value, &m_write_frame_info.reg_values[i * 2]);
                }
                break;
            }
            case Format_32_Bit_Float_Little_Endian_Byte_Swap:
            {
                m_write_frame_info.quantity = m_function_16_data.quantity * 2;
                for(int i = 0; i < m_function_16_data.quantity; ++i)
                {
                    snprintf(m_function_16_data.value_name[i], sizeof(m_function_16_data.value_name[i]), "%d", i + m_function_16_data.address);
                    ImGui::InputFloat(m_function_16_data.value_name[i], &m_function_16_data.values[i].f32_value);
                    myToLittleEndian(m_function_16_data.values[i].f32_value, &m_write_frame_info.reg_values[i * 2]);
                }
                break;
            }
            case Format_64_Bit_Float_Big_Endian:
            {
                m_write_frame_info.quantity = m_function_16_data.quantity * 4;
                for(int i = 0; i < m_function_16_data.quantity; ++i)
                {
                    snprintf(m_function_16_data.value_name[i], sizeof(m_function_16_data.value_name[i]), "%d", i + m_function_16_data.address);
                    ImGui::InputDouble(m_function_16_data.value_name[i], &m_function_16_data.values[i].f64_value);
                    myToBigEndianByteSwap(m_function_16_data.values[i].f64_value, &m_write_frame_info.reg_values[i * 4]);
                }
                break;
            }
            case Format_64_Bit_Float_Little_Endian:
            {
                m_write_frame_info.quantity = m_function_16_data.quantity * 4;
                for(int i = 0; i < m_function_16_data.quantity; ++i)
                {
                    snprintf(m_function_16_data.value_name[i], sizeof(m_function_16_data.value_name[i]), "%d", i + m_function_16_data.address);
                    ImGui::InputDouble(m_function_16_data.value_name[i], &m_function_16_data.values[i].f64_value);
                    myToLittleEndianByteSwap(m_function_16_data.values[i].f64_value, &m_write_frame_info.reg_values[i * 4]);
                }
                break;
            }
            case Format_64_Bit_Float_Big_Endian_Byte_Swap:
            {
                m_write_frame_info.quantity = m_function_16_data.quantity * 4;
                for(int i = 0; i < m_function_16_data.quantity; ++i)
                {
                    snprintf(m_function_16_data.value_name[i], sizeof(m_function_16_data.value_name[i]), "%d", i + m_function_16_data.address);
                    ImGui::InputDouble(m_function_16_data.value_name[i], &m_function_16_data.values[i].f64_value);
                    myToBigEndian(m_function_16_data.values[i].f64_value, &m_write_frame_info.reg_values[i * 4]);
                }
                break;
            }
            case Format_64_Bit_Float_Little_Endian_Byte_Swap:
            {
                m_write_frame_info.quantity = m_function_16_data.quantity * 4;
                for(int i = 0; i < m_function_16_data.quantity; ++i)
                {
                    snprintf(m_function_16_data.value_name[i], sizeof(m_function_16_data.value_name[i]), "%d", i + m_function_16_data.address);
                    ImGui::InputDouble(m_function_16_data.value_name[i], &m_function_16_data.values[i].f64_value);
                    myToLittleEndian(m_function_16_data.values[i].f64_value, &m_write_frame_info.reg_values[i * 4]);
                }
                break;
            }
            default:
            {
                break;
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();
        ImGui::BeginGroup();
        ImGui::BeginChild("right_panel", ImVec2(ImGui::GetWindowWidth() / 4, 0));
        if(ImGui::Button(gettext("Send"), ImVec2(ImGui::GetWindowWidth(), 35)))
        {
            ModbusPacket *mdb_pack = new ModbusPacket;
            mdb_pack->packet_size = m_modbus->masterFrame2Pack(m_write_frame_info, mdb_pack->packet);
            m_manual_list.push_back(mdb_pack);
        }
        if(ImGui::Button(gettext("Cancel"), ImVec2(ImGui::GetWindowWidth(), 35)))
        {
            m_modbus_function_16_dialog_visible = false;
        }
        ImGui::EndChild();
        ImGui::EndGroup();
    }
    ImGui::End();
}

void ModbusWindow::read_data_callback(const char *buffer, size_t buffer_size)
{
    if(m_modbus->validPack(buffer, buffer_size))
    {
        ModbusFrameInfo frame_info{};
        if (m_identifier == ModbusMaster)
        {
            char msg[1024];
            size_t str_size = toHexString((const uint8_t *)buffer, buffer_size, msg);
            LogInfo("<< {}", msg);
            frame_info = m_modbus->masterPack2Frame(buffer, buffer_size);
            m_master_last_send_frame = m_modbus->slavePack2Frame(m_master_last_send_data->packet, m_master_last_send_data->packet_size);
            LogInfo("Frame_info.id: {}, last_send_frame.id:{}", frame_info.id, m_master_last_send_frame.id);
            if (frame_info.id == m_master_last_send_frame.id)
            {
                LogInfo("frame_info.trans_id:{},last_send_frame.trans_id:{}", frame_info.trans_id, m_master_last_send_frame.trans_id);
                if (((m_protocol == MODBUS_TCP || m_protocol == MODBUS_UDP) && frame_info.trans_id == m_master_last_send_frame.trans_id) ||
                    (m_protocol != MODBUS_TCP && m_protocol != MODBUS_UDP))
                {
                    if (m_communication_traffic_dialog_visible && !m_communication_traffic_window_data.stopped)
                    {
                        msg[str_size++] = '\n';
                        msg[str_size++] = '\0';
                        char time_stamp[32] = "";
                        if (m_communication_traffic_window_data.timestamp)
                        {
                            getTimeStampString(time_stamp, sizeof(time_stamp));
                        }
                        m_communication_traffic_window_data.communication_traffic_text.append(time_stamp).append("Rx : ").append(msg);
                        if (m_communication_traffic_window_data.stop_on_error)
                        {
                            m_communication_traffic_window_data.stopped = frame_info.function > ModbusFunctionError;
                        }
                    }
                    SDL_RemoveTimer(m_recv_timer_id);
                    process_master_frame(frame_info);
                    m_send_timer_id = SDL_AddTimer(1, Send_timer_callback, this);
                }
            }
        }
        else if (m_identifier == ModbusSlave)
        {
            frame_info = m_modbus->slavePack2Frame(buffer, buffer_size);
            ModbusErrorCode error_code { ModbusErrorCode_OK };
            RegistersTableData *slave_reg_table_data = getSlaveReadTableData(frame_info.id, frame_info.function, frame_info.reg_addr, frame_info.reg_addr + frame_info.quantity - 1, error_code);
            if(slave_reg_table_data)
            {
                if(m_communication_traffic_dialog_visible && !m_communication_traffic_window_data.stopped)
                {
                    char msg[1024];
                    size_t str_size = toHexString((const uint8_t *)buffer, buffer_size, msg);
                    LogInfo("<< {}", msg);
                    msg[str_size++] = '\n';
                    msg[str_size++] = '\0';
                    char time_stamp[32] = "";
                    if (m_communication_traffic_window_data.timestamp)
                    {
                        getTimeStampString(time_stamp, sizeof(time_stamp));
                    }
                    m_communication_traffic_window_data.communication_traffic_text.append(time_stamp).append("Rx : ").append(msg);
                }
                process_slave_frame(frame_info, slave_reg_table_data, error_code);
            }
        }
        m_myIODevice->clear();
    }
}

uint32_t ModbusWindow::scan_timer_callback(uint32_t interval, void *param)
{
    if (!m_cycle_list.empty())
    {
        return interval;
    }
    uint32_t tick = SDL_GetTicks();
    for (auto &regs_table_data : m_registers_table_datas)
    {
        if (tick - m_last_scan_timestamp_map[regs_table_data] >= regs_table_data->scan_rate)
        {
            ModbusPacket *mdb_pack = new ModbusPacket;
            if(regs_table_data->function == ModbusReadCoils || regs_table_data->function == ModbusReadDescreteInputs
                || regs_table_data->function == ModbusReadHoldingRegisters || regs_table_data->function == ModbusReadInputRegisters)
            {
                memcpy(mdb_pack->packet, regs_table_data->packet, regs_table_data->packet_size);
                mdb_pack->packet_size = regs_table_data->packet_size;
            }
            else
            {
                ModbusFrameInfo frame_info{};
                frame_info.id = regs_table_data->id;
                frame_info.function = regs_table_data->function;
                frame_info.reg_addr = regs_table_data->reg_start;
                frame_info.quantity = regs_table_data->reg_quantity;
                memcpy(frame_info.reg_values, regs_table_data->reg_values, regs_table_data->reg_quantity * sizeof(uint16_t));
                mdb_pack->packet_size = m_modbus->masterFrame2Pack(frame_info, mdb_pack->packet);
            }
            m_cycle_list.push_back(mdb_pack);
            m_cycle_table_list.push_back(regs_table_data);
            m_last_scan_timestamp_map[regs_table_data] = tick;
        }
    }
    return interval;
}

uint32_t ModbusWindow::send_timer_callback(uint32_t interval, void *param)
{
    bool has_pack = false;
    RegistersTableData *regs_table_data = nullptr;
    if (!m_manual_list.empty())
    {
        has_pack = true;
        m_master_last_send_data = m_manual_list.front();
    }
    else if (!m_cycle_list.empty())
    {
        has_pack = true;
        m_master_last_send_data = m_cycle_list.front();
        regs_table_data = m_cycle_table_list.front();
        regs_table_data->send_count++;
        regs_table_data->update_info();
    }
    if (has_pack)
    {
        if (m_protocol == MODBUS_TCP || m_protocol == MODBUS_UDP)
        {
            setModbusPacketTransID(m_master_last_send_data->packet, m_trans_id);
            m_trans_id++;
        }
        m_myIODevice->write(m_master_last_send_data->packet, m_master_last_send_data->packet_size);
        SDL_RemoveTimer(m_send_timer_id);
        m_recv_timer_id = SDL_AddTimer(m_recv_timeout_ms, Recv_timer_callback, this);
        char msg[1024];
        size_t str_size = toHexString((const uint8_t *)m_master_last_send_data->packet, m_master_last_send_data->packet_size, msg);
        LogInfo(">> {}", msg);
        if (m_communication_traffic_dialog_visible && !m_communication_traffic_window_data.stopped)
        {
            msg[str_size++] = '\n';
            msg[str_size++] = '\0';
            char time_stamp[32] = "";
            if (m_communication_traffic_window_data.timestamp)
            {
                getTimeStampString(time_stamp, sizeof(time_stamp));
            }
            m_communication_traffic_window_data.communication_traffic_text.append(time_stamp).append("Tx : ").append(msg);
        }
    }
    return interval;
}

uint32_t ModbusWindow::recv_timer_callback(uint32_t interval, void *param)
{
    SDL_RemoveTimer(m_recv_timer_id);
    RegistersTableData *regs_table_data = nullptr;
    bool is_manual_frame{ false };
    if (!m_manual_list.empty())
    {
        is_manual_frame = true;
        delete m_manual_list.front();
        m_manual_list.pop_front();
    }
    else if (!m_cycle_list.empty())
    {
        delete m_cycle_list.front();
        m_cycle_list.pop_front();
        regs_table_data = m_cycle_table_list.front();
        m_cycle_table_list.pop_front();
        regs_table_data->error_count++;
        regs_table_data->update_info();
        snprintf(regs_table_data->msg, sizeof(regs_table_data->msg), "Timeout Error");
        LogInfo("Timeout Error");
    }
    m_master_last_send_frame = m_modbus->slavePack2Frame(m_master_last_send_data->packet, m_master_last_send_data->packet_size);
    if ((m_master_last_send_frame.function == ModbusWriteSingleCoil || m_master_last_send_frame.function == ModbusWriteMultipleCoils || m_master_last_send_frame.function == ModbusWriteSingleRegister || m_master_last_send_frame.function == ModbusWriteMultipleRegisters) && is_manual_frame)
    {
        if (m_write_frame_response_callback)
        {
            m_write_frame_response_callback(ModbusErrorCode_Timeout);
        }
    }
    m_error_count_map[ModbusErrorCode_Timeout]++;
    m_myIODevice->clear();
    m_send_timer_id = SDL_AddTimer(1, Send_timer_callback, this);
    return interval;
}

void ModbusWindow::error_handle(const char *error_msg)
{
    LogError("{}", error_msg);
}

void ModbusWindow::write_master_register_value(CellFormat format, const char *value_str, RegistersTableData *reg_table_data, int reg_index)
{
    ModbusFrameInfo frame_info{};
    bool data_valid = false;
    frame_info.id = reg_table_data->id;
    frame_info.reg_addr = reg_table_data->reg_start + reg_index;
    switch (format)
    {
    case Format_None:
    {
        break;
    }
    case Format_Coil:
    {
        data_valid = true;
        frame_info.function = ModbusWriteSingleCoil;
        frame_info.quantity = 1;
        frame_info.reg_values[0] = (strcmp(value_str, "0") != 0);
        break;
    }
    case Format_Unsigned:
    case Format_Signed:
    {
        try
        {
            frame_info.reg_values[0] = std::stoi(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteSingleRegister;
            frame_info.quantity = 1;
        }
        catch (std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_Ascii_Hex:
    {
        break;
    }
    case Format_Hex:
    {
        try
        {
            frame_info.reg_values[0] = std::stoi(value_str, nullptr, 16);
            data_valid = true;
            frame_info.function = ModbusWriteSingleRegister;
            frame_info.quantity = 1;
        }
        catch (std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_Binary:
    {
        try
        {
            frame_info.reg_values[0] = std::stoi(value_str, nullptr, 2);
            data_valid = true;
            frame_info.function = ModbusWriteSingleRegister;
            frame_info.quantity = 1;
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Unsigned_Big_Endian:
    case Format_32_Bit_Signed_Big_Endian:
    {
        try
        {
            int32_t value = std::stoi(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 2;
            myToBigEndianByteSwap(value, frame_info.reg_values);
        }
        catch (std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Unsigned_Little_Endian:
    case Format_32_Bit_Signed_Little_Endian:
    {
        try
        {
            int32_t value = std::stoi(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 2;
            myToLittleEndianByteSwap(value, frame_info.reg_values);
        }
        catch (std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Unsigned_Big_Endian_Byte_Swap:
    case Format_32_Bit_Signed_Big_Endian_Byte_Swap:
    {
        try
        {
            int32_t value = std::stoi(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 2;
            myToBigEndian(value, frame_info.reg_values);
        }
        catch (std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Unsigned_Little_Endian_Byte_Swap:
    case Format_32_Bit_Signed_Little_Endian_Byte_Swap:
    {
        try
        {
            int32_t value = std::stoi(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 2;
            myToLittleEndian(value, frame_info.reg_values);
        }
        catch (std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_64_Bit_Unsigned_Big_Endian:
    case Format_64_Bit_Signed_Big_Endian:
    {
        try
        {
            int64_t value = std::stoll(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 4;
            myToBigEndianByteSwap(value, frame_info.reg_values);
        }
        catch (std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_64_Bit_Unsigned_Little_Endian:
    case Format_64_Bit_Signed_Little_Endian:
    {
        try
        {
            int64_t value = std::stoll(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 4;
            myToLittleEndianByteSwap(value, frame_info.reg_values);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_64_Bit_Unsigned_Big_Endian_Byte_Swap:
    case Format_64_Bit_Signed_Big_Endian_Byte_Swap:
    {
        try
        {
            int64_t value = std::stoll(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 4;
            myToBigEndian(value, frame_info.reg_values);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_64_Bit_Unsigned_Little_Endian_Byte_Swap:
    case Format_64_Bit_Signed_Little_Endian_Byte_Swap:
    {
        try
        {
            int64_t value = std::stoll(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 4;
            myToLittleEndian(value, frame_info.reg_values);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Float_Big_Endian:
    {
        try
        {
            float value = std::stof(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 2;
            myToBigEndianByteSwap(value, frame_info.reg_values);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Float_Little_Endian:
    {
        try
        {
            float value = std::stof(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 2;
            myToLittleEndianByteSwap(value, frame_info.reg_values);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Float_Big_Endian_Byte_Swap:
    {
        try
        {
            float value = std::stof(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 2;
            myToBigEndian(value, frame_info.reg_values);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_32_Bit_Float_Little_Endian_Byte_Swap:
    {
        try
        {
            float value = std::stof(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 2;
            myToLittleEndian(value, frame_info.reg_values);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_64_Bit_Float_Big_Endian:
    {
        try
        {
            double value = std::stod(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 4;
            myToBigEndianByteSwap(value, frame_info.reg_values);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_64_Bit_Float_Little_Endian:
    {
        try
        {
            double value = std::stod(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 4;
            myToLittleEndianByteSwap(value, frame_info.reg_values);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_64_Bit_Float_Big_Endian_Byte_Swap:
    {
        try
        {
            double value = std::stod(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 4;
            myToBigEndian(value, frame_info.reg_values);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    case Format_64_Bit_Float_Little_Endian_Byte_Swap:
    {
        try
        {
            double value = std::stod(value_str);
            data_valid = true;
            frame_info.function = ModbusWriteMultipleRegisters;
            frame_info.quantity = 4;
            myToLittleEndian(value, frame_info.reg_values);
        }
        catch (const std::exception &e)
        {
            error_handle(e.what());
        }
        break;
    }
    default:
        break;
    }
    if (data_valid)
    {
        ModbusPacket *mdb_packet = new ModbusPacket();
        mdb_packet->packet_size = m_modbus->masterFrame2Pack(frame_info, mdb_packet->packet);
        m_manual_list.push_back(mdb_packet);
    }
}

void ModbusWindow::process_master_frame(const ModbusFrameInfo &frame_info)
{
    RegistersTableData *regs_table_data = nullptr;
    bool is_manual_frame{ false };
    if (!m_manual_list.empty())
    {
        is_manual_frame = true;
        delete m_manual_list.front();
        m_manual_list.pop_front();
    }
    else if (!m_cycle_list.empty())
    {
        delete m_cycle_list.front();
        m_cycle_list.pop_front();
        regs_table_data = m_cycle_table_list.front();
        m_cycle_table_list.pop_front();
    }
    if (frame_info.function > ModbusFunctionError)
    {
        ModbusErrorCode error_code = (ModbusErrorCode)frame_info.reg_values[0];
        int func_code = frame_info.function - ModbusFunctionError;
        if ((func_code == ModbusWriteSingleCoil || func_code == ModbusWriteMultipleCoils ||
            func_code == ModbusWriteSingleRegister || func_code == ModbusWriteMultipleRegisters) && is_manual_frame)
        {
            if (m_write_frame_response_callback)
            {
                m_write_frame_response_callback(error_code);
            }
        }
        m_error_count_map[error_code]++;
        if (regs_table_data)
        {
            regs_table_data->error_count++;
            regs_table_data->update_info();
        }
        snprintf(regs_table_data->msg, sizeof(regs_table_data->msg), "%s", modbus_error_code_map[error_code]);
    }
    else if (frame_info.function == ModbusReadCoils || frame_info.function == ModbusReadDescreteInputs)
    {
        uint8_t *coils = (uint8_t *)frame_info.reg_values;
        for (int i = 0; i < m_master_last_send_frame.quantity; ++i)
        {
            int byte_index = i / 8;
            int bit_index = i % 8;
            regs_table_data->reg_values[i] = getBit(coils[byte_index], bit_index);
        }
        regs_table_data->msg[0] = '\0';
    }
    else if (frame_info.function == ModbusReadHoldingRegisters || frame_info.function == ModbusReadInputRegisters)
    {
        memcpy(regs_table_data->reg_values + (m_master_last_send_frame.reg_addr - regs_table_data->reg_start),
               frame_info.reg_values, frame_info.quantity * sizeof(frame_info.reg_values[0]));
        regs_table_data->msg[0] = '\0';
    }
    else if ((frame_info.function == ModbusWriteSingleCoil || frame_info.function == ModbusWriteMultipleCoils ||
             frame_info.function == ModbusWriteSingleRegister || frame_info.function == ModbusWriteMultipleRegisters) && is_manual_frame)
    {
        if (m_write_frame_response_callback)
        {
            m_write_frame_response_callback(ModbusErrorCode_OK);
        }
    }
    else
    {
        LogWarn("Unknown Function:{}", frame_info.function);
    }
}

void ModbusWindow::process_slave_frame(const ModbusFrameInfo &frame_info, RegistersTableData *slave_reg_table_data, ModbusErrorCode &error_code)
{
    ModbusFrameInfo reply_frame {};
    reply_frame.id = frame_info.id;
    reply_frame.trans_id = frame_info.trans_id;
    reply_frame.function = frame_info.function;
    reply_frame.reg_addr = frame_info.reg_addr;
    reply_frame.quantity = frame_info.quantity;
    if(frame_info.function == ModbusReadHoldingRegisters || frame_info.function == ModbusReadInputRegisters)
    {
        memcpy(reply_frame.reg_values, slave_reg_table_data->reg_values + (frame_info.reg_addr - slave_reg_table_data->reg_start), frame_info.quantity * sizeof(frame_info.reg_values[0]));
    }
    else if(frame_info.function == ModbusReadCoils || frame_info.function == ModbusReadDescreteInputs)
    {
        uint8_t *coils = (uint8_t *)reply_frame.reg_values;
        for(int i = 0; i < frame_info.quantity; ++i)
        {
            int byte_index = i / 8;
            int bit_index = i % 8;
            setBit(coils[byte_index], bit_index, slave_reg_table_data->reg_values[i + (frame_info.reg_addr - slave_reg_table_data->reg_start)]);
        }
    }
    else if(frame_info.function == ModbusWriteSingleCoil)
    {
        reply_frame.reg_values[0] = frame_info.reg_values[0];
        slave_reg_table_data->reg_values[frame_info.reg_addr - slave_reg_table_data->reg_start] = frame_info.reg_values[0] >> 8 & 0xFF ? 1 : 0;
    }
    else if(frame_info.function == ModbusWriteMultipleCoils)
    {
        uint8_t *coils = (uint8_t *)frame_info.reg_values;
        for(int i = 0; i < frame_info.quantity; ++i)
        {
            int byte_index = i / 8;
            int bit_index = i % 8;
            slave_reg_table_data->reg_values[frame_info.reg_addr - slave_reg_table_data->reg_start + i] = getBit(coils[byte_index], bit_index);
        }
    }
    else if(frame_info.function == ModbusWriteSingleRegister)
    {
        reply_frame.reg_values[0] = frame_info.reg_values[0];
        slave_reg_table_data->reg_values[frame_info.reg_addr - slave_reg_table_data->reg_start] = frame_info.reg_values[0];
    }
    else if(frame_info.function == ModbusWriteMultipleRegisters)
    {
        memcpy(slave_reg_table_data->reg_values + (frame_info.reg_addr - slave_reg_table_data->reg_start), frame_info.reg_values, frame_info.quantity * sizeof(frame_info.reg_values[0]));
    }
    else 
    {
        reply_frame.function = frame_info.function + ModbusFunctionError;
        reply_frame.reg_values[0] = error_code;
    }
    ModbusPacket reply_packet;
    reply_packet.packet_size = m_modbus->slaveFrame2Pack(reply_frame, reply_packet.packet);
    m_myIODevice->write(reply_packet.packet, reply_packet.packet_size);
}

RegistersTableData *ModbusWindow::getSlaveReadTableData(int id, int function, int reg_start, int reg_end, ModbusErrorCode &error_code)
{
    bool found_function{ false };
    bool found_reg_address{ false };
    RegistersTableData *ret{ nullptr };
    for(auto x : m_registers_table_datas)
    {
        if(x->id == id)
        {
            if(x->function == function
                || ((function == ModbusWriteSingleCoil || function == ModbusWriteMultipleCoils) && x->function == ModbusCoilStatus)
                || ((function == ModbusWriteSingleRegister || function == ModbusWriteMultipleRegisters) && x->function == ModbusHoldingRegisters))
            {
                found_function = true;
                if(x->reg_start <= reg_start && x->reg_end >= reg_end)
                {
                    found_reg_address = true;
                    ret = x;
                }
            }
        }
    }
    if(!found_function)
    {
        error_code = ModbusErrorCode_Illegal_Function;
    }
    else if(!found_reg_address)
    {
        error_code = ModbusErrorCode_Illegal_Data_Address;
    }
    return ret;
}

uint32_t Scan_timer_callback(uint32_t interval, void *param)
{
    ModbusWindow *window = (ModbusWindow *)param;
    return window->scan_timer_callback(interval, param);
}

uint32_t Send_timer_callback(uint32_t interval, void *param)
{
    ModbusWindow *window = (ModbusWindow *)param;
    return window->send_timer_callback(interval, param);
}

uint32_t Recv_timer_callback(uint32_t interval, void *param)
{
    ModbusWindow *window = (ModbusWindow *)param;
    return window->recv_timer_callback(interval, param);
}
