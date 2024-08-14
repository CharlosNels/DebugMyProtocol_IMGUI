#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <unordered_map>
#include <imgui.h>
#include <CSerialPort/SerialPort.h>
#include <vector>
#include "utils.h"
#include "ModbusFrameInfo.h"
#include "ModbusBase.h"

class MyTcpSocket;


class ModbusWindow;

class MainWindow
{

    enum RouteType{
        RouteType_SerialPort,
        RouteType_TCP_Server,
        RouteType_TCP_Client,
        RouteType_UDP,
    };

public:
    MainWindow(bool *should_close);
    ~MainWindow();

    void render();

private:
    void render_serial_port_route();
    void render_tcp_server_route();
    void render_tcp_client_route();
    void render_udp_route();
    void error_callback(const char *msg);
    void tcp_new_connection_callback(MyTcpSocket *socket, MyTcpSocket *server);
    void tcp_connected_callback(bool connected);


private:
    ComboBoxData m_protocol_combo_box_data;
    ComboBoxData m_identifier_combo_box_data;
    ComboBoxData m_serial_port_name_combo_box_data;
    ComboBoxData m_serial_port_baud_rate_combo_box_data;
    ComboBoxData m_serial_port_data_bits_combo_box_data;
    ComboBoxData m_serial_port_stop_bits_combo_box_data;
    ComboBoxData m_serial_port_parity_combo_box_data;
    ComboBoxData m_serial_port_flow_control_combo_box_data;
    int m_tcp_server_port;
    char m_tcp_client_addr[32];
    int m_tcp_client_remote_port;
    char m_udp_addr[32];
    int m_udp_port;
    RouteType m_route_type;
    bool *m_should_close;
    std::unordered_map<const char *, itas109::BaudRate> baud_rate_map;
    std::unordered_map<const char *, itas109::DataBits> data_bits_map;
    std::unordered_map<const char *, itas109::StopBits> stop_bits_map;
    std::unordered_map<const char *, itas109::Parity> parity_map;
    std::unordered_map<const char *, itas109::FlowControl> flow_control_map;
    std::unordered_map<const char *, ModbusIdentifier> identifier_map;
    std::unordered_map<const char *, ModbusBase *> modbus_map;
    std::unordered_map<const char *, Protocols> protocol_map;
    std::vector<ModbusWindow *> m_modbus_windows;
    std::unordered_map<MyTcpSocket *, ModbusIdentifier> m_tcp_server_identifier_map;
    std::unordered_map<MyTcpSocket *, const char *> m_tcp_server_protocol_map;
    MyTcpSocket *m_connecting_client;
};

#endif