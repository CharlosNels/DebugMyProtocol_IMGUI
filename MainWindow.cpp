#include "MainWindow.h"
#include "ModbusFrameInfo.h"
#include "ModbusWindow.h"
#include "myudpsocket.h"
#include "utils.h"
#include "MySerialPort.h"
#include "mytcpsocket.h"
#include "modbus_ascii.h"
#include "modbus_rtu.h"
#include "modbus_tcp.h"
#include <imgui.h>
#include <unordered_map>
#include <boost/asio.hpp>
#include <libintl.h>
#include <CSerialPort/SerialPort.h>
#include <CSerialPort/SerialPortInfo.h>
#include <CSerialPort/SerialPortListener.h>

const char *protocols[] = {"Modbus UDP", "Modbus RTU", "Modbus ASCII", "Modbus TCP"};

MainWindow::MainWindow(bool *should_close)
{
    m_tcp_server_port = 19980;
    memset(m_tcp_client_addr, 0, sizeof(m_tcp_client_addr));
    m_tcp_client_remote_port = 19980;
    memset(m_udp_addr, 0, sizeof(m_udp_addr));
    m_udp_port = 19980;
    m_route_type = RouteType_SerialPort;
    m_should_close = should_close;
    baud_rate_map = {
        {"1200", itas109::BaudRate1200},
        {"2400", itas109::BaudRate2400},
        {"4800", itas109::BaudRate4800},
        {"9600", itas109::BaudRate9600},
        {"19200", itas109::BaudRate19200},
        {"38400", itas109::BaudRate38400},
        {"57600", itas109::BaudRate57600},
        {"115200", itas109::BaudRate115200}};

    data_bits_map = {
        {"5", itas109::DataBits5},
        {"6", itas109::DataBits6},
        {"7", itas109::DataBits7},
        {"8", itas109::DataBits8}};

    stop_bits_map = {
        {"1", itas109::StopOne},
        {"1.5", itas109::StopOneAndHalf},
        {"2", itas109::StopTwo}};

    parity_map = {
        {gettext("None"), itas109::ParityNone},
        {gettext("Odd"), itas109::ParityOdd},
        {gettext("Even"), itas109::ParityEven}};

    flow_control_map = {
        {gettext("None"), itas109::FlowNone},
        {gettext("Hardware"), itas109::FlowHardware},
        {gettext("Software"), itas109::FlowSoftware}};

    identifier_map = {
        {gettext("Modbus Master"), ModbusMaster},
        {gettext("Modbus Slave"), ModbusSlave}};
    modbus_map = {
        {"Modbus UDP", new Modbus_TCP()},
        {"Modbus TCP", new Modbus_TCP()},
        {"Modbus RTU", new Modbus_RTU()},
        {"Modbus ASCII", new Modbus_ASCII()}};
    protocol_map = {
        {"Modbus UDP", MODBUS_UDP},
        {"Modbus TCP", MODBUS_TCP},
        {"MODBUS ASCII", MODBUS_ASCII},
        {"MODBUS RTU", MODBUS_RTU}};
}

MainWindow::~MainWindow()
{
    for(auto iter = m_modbus_windows.begin(); iter != m_modbus_windows.end(); ++iter)
    {
        delete *iter;
    }
    for(auto iter = modbus_map.begin(); iter != modbus_map.end(); ++iter)
    {
        delete iter->second;
    }
}

void MainWindow::render()
{
    ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
    static bool open = true;
    if (!open)
    {
        *m_should_close = true;
    }
    if (ImGui::Begin("Debug My Protocol", &open))
    {
        int protocol_start = 0, protocol_end = 0;
        switch (m_route_type)
        {
        case RouteType_SerialPort:
        {
            protocol_start = 1;
            protocol_end = 2;
            break;
        }
        case RouteType_TCP_Client:
        case RouteType_TCP_Server:
        {
            protocol_start = 1;
            protocol_end = 3;
            break;
        }
        case RouteType_UDP:
        {
            protocol_start = 0;
            protocol_end = 2;
            break;
        }
        default:
        {
            break;
        }
        }
        render_combo_box(m_protocol_combo_box_data, gettext("Protocol"), protocols, protocol_start, protocol_end);
        render_combo_box(m_identifier_combo_box_data, gettext("Identify"), (const char **)getKeysOfMap(identifier_map).data(), 0, 1);
        render_serial_port_route();
        render_tcp_server_route();
        render_tcp_client_route();
        render_udp_route();
    }
    ImGui::End();
    for (auto iter = m_modbus_windows.begin(); iter != m_modbus_windows.end();)
    {
        if ((*iter)->visible())
        {
            (*iter)->render();
            ++iter;
        }
        else
        {
            delete *iter;
            iter = m_modbus_windows.erase(iter);
        }
    }
}

void MainWindow::render_serial_port_route()
{
    if (m_route_type != RouteType_SerialPort)
    {
        ImGui::SetNextItemOpen(false);
    }
    if (!ImGui::CollapsingHeader("Serial Port"))
    {
        return;
    }
    m_route_type = RouteType_SerialPort;
    using namespace itas109;
    std::vector<SerialPortInfo> serial_port_info_list = CSerialPortInfo::availablePortInfos();
    const char **serial_port_name_list = new const char *[serial_port_info_list.size()];
    for (int i = 0; i < serial_port_info_list.size(); ++i)
    {
        serial_port_name_list[i] = serial_port_info_list[i].portName;
    }
    render_combo_box(m_serial_port_name_combo_box_data, gettext("Port Name"), serial_port_name_list, 0, serial_port_info_list.size() - 1);
    render_combo_box(m_serial_port_baud_rate_combo_box_data, gettext("Baud Rate"), getKeysOfMap(baud_rate_map).data(), 0, baud_rate_map.size() - 1);
    render_combo_box(m_serial_port_data_bits_combo_box_data, gettext("Data Bits"), getKeysOfMap(data_bits_map).data(), 0, data_bits_map.size() - 1);
    render_combo_box(m_serial_port_stop_bits_combo_box_data, gettext("Stop Bits"), getKeysOfMap(stop_bits_map).data(), 0, stop_bits_map.size() - 1);
    render_combo_box(m_serial_port_parity_combo_box_data, gettext("Parity"), getKeysOfMap(parity_map).data(), 0, parity_map.size() - 1);
    render_combo_box(m_serial_port_flow_control_combo_box_data, gettext("Flow Control"), getKeysOfMap(flow_control_map).data(), 0, flow_control_map.size() - 1);
    if (ImGui::Button(gettext("Open"), ImVec2(ImGui::GetWindowContentRegionMax().x - 50, 35)))
    {
        CSerialPort *cserial_port = new CSerialPort();
        cserial_port->setPortName(m_serial_port_name_combo_box_data.text);
        cserial_port->setBaudRate(baud_rate_map[m_serial_port_baud_rate_combo_box_data.text]);
        cserial_port->setDataBits(data_bits_map[m_serial_port_data_bits_combo_box_data.text]);
        cserial_port->setStopBits(stop_bits_map[m_serial_port_stop_bits_combo_box_data.text]);
        cserial_port->setParity(parity_map[m_serial_port_parity_combo_box_data.text]);
        cserial_port->setFlowControl(flow_control_map[m_serial_port_flow_control_combo_box_data.text]);
        MySerialPort *serial_port = new MySerialPort(cserial_port);
        ModbusWindow *modbus_window = new ModbusWindow(serial_port, m_serial_port_name_combo_box_data.text, identifier_map[m_identifier_combo_box_data.text], protocol_map[m_protocol_combo_box_data.text], modbus_map[m_protocol_combo_box_data.text]);
        m_modbus_windows.push_back(modbus_window);
    }
    delete[] serial_port_name_list;
}

void MainWindow::render_tcp_server_route()
{
    if (m_route_type != RouteType_TCP_Server)
    {
        ImGui::SetNextItemOpen(false);
    }
    if (!ImGui::CollapsingHeader("TCP Server"))
    {
        return;
    }
    m_route_type = RouteType_TCP_Server;
    ImGui::InputInt(gettext("Port"), &m_tcp_server_port, 1, 100);
    if (ImGui::Button(gettext("Listen"), ImVec2(ImGui::GetWindowContentRegionMax().x - 50, 35)))
    {
        MyTcpSocket *tcp_server = new MyTcpSocket();
        m_tcp_server_identifier_map[tcp_server] = identifier_map[m_identifier_combo_box_data.text];
        m_tcp_server_protocol_map[tcp_server] = m_protocol_combo_box_data.text;
        tcp_server->bind(m_tcp_server_port);
        tcp_server->setNewConnectionCallback(std::bind(&MainWindow::tcp_new_connection_callback, this, std::placeholders::_1, std::placeholders::_2));
    }
}

void MainWindow::render_tcp_client_route()
{
    if (m_route_type != RouteType_TCP_Client)
    {
        ImGui::SetNextItemOpen(false);
    }
    if (!ImGui::CollapsingHeader("TCP Client"))
    {
        return;
    }
    m_route_type = RouteType_TCP_Client;
    ImGui::InputText(gettext("Address"), m_tcp_client_addr, sizeof(m_tcp_client_addr));
    ImGui::InputInt(gettext("Port"), &m_tcp_client_remote_port, 1, 100);
    if (ImGui::Button(gettext("Connect"), ImVec2(ImGui::GetWindowContentRegionMax().x - 50, 35)))
    {
        m_connecting_client = new MyTcpSocket();
        m_connecting_client->setConnectedCallback(std::bind(&MainWindow::tcp_connected_callback, this, std::placeholders::_1));
        m_connecting_client->connectToHost(m_tcp_client_addr, m_tcp_client_remote_port);
    }
}

void MainWindow::render_udp_route()
{
    if (m_route_type != RouteType_UDP)
    {
        ImGui::SetNextItemOpen(false);
    }
    if (!ImGui::CollapsingHeader("UDP"))
    {
        return;
    }
    m_route_type = RouteType_UDP;
    ImGui::InputText(gettext("Address"), m_udp_addr, sizeof(m_udp_addr));
    ImGui::InputInt(gettext("Port"), &m_udp_port, 1, 100);
    if (ImGui::Button(gettext("Connect"), ImVec2(ImGui::GetWindowContentRegionMax().x - 50, 35)))
    {
        MyUdpSocket *udp_socket = new MyUdpSocket();
        bool socket_create_successed = false;
        char window_name[128];
        if (identifier_map[m_identifier_combo_box_data.text] == ModbusMaster)
        {
            socket_create_successed = udp_socket->connectTo(m_udp_addr, m_udp_port);
            snprintf(window_name, sizeof(window_name), "UDP %s:%u", m_udp_addr, m_udp_port);
        }
        else if (identifier_map[m_identifier_combo_box_data.text] == ModbusSlave)
        {
            socket_create_successed = udp_socket->bind(m_udp_port);
            snprintf(window_name, sizeof(window_name), "UDP localhost:%u", m_udp_port);
        }
        if (socket_create_successed)
        {
            udp_socket->setErrorCallback(std::bind(&MainWindow::error_callback, this, std::placeholders::_1));
            ModbusWindow *modbus_window = new ModbusWindow(udp_socket, window_name, identifier_map[m_identifier_combo_box_data.text], protocol_map[m_protocol_combo_box_data.text], modbus_map[m_protocol_combo_box_data.text]);
            m_modbus_windows.push_back(modbus_window);
        }
    }
}

void MainWindow::error_callback(const char *msg)
{
    LogError(msg);
}

void MainWindow::tcp_new_connection_callback(MyTcpSocket *socket, MyTcpSocket *server)
{
    char window_name[128];
    snprintf(window_name, sizeof(window_name), "%s:%u", socket->peerAddress(), socket->peerPort());
    ModbusWindow *modbus_window = new ModbusWindow(socket, window_name, m_tcp_server_identifier_map[server], protocol_map[m_tcp_server_protocol_map[server]], modbus_map[m_tcp_server_protocol_map[server]]);
    m_modbus_windows.push_back(modbus_window);
}

void MainWindow::tcp_connected_callback(bool connected)
{
    if (connected)
    {
        char window_name[128];
        snprintf(window_name, sizeof(window_name), "%s:%u", m_connecting_client->peerAddress(), m_connecting_client->peerPort());
        ModbusWindow *modbus_window = new ModbusWindow(m_connecting_client, window_name, ModbusMaster, protocol_map[m_protocol_combo_box_data.text], modbus_map[m_protocol_combo_box_data.text]);
        m_modbus_windows.push_back(modbus_window);
    }
}
