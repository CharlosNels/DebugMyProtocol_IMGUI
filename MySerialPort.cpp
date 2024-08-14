#include "MySerialPort.h"
#include "utils.h"
MySerialPort::MySerialPort(itas109::CSerialPort *serial_port) : m_serial_port(serial_port)
{
    m_recv_buffer = new char[1024];
    m_recv_buffer_size = 0;
}

MySerialPort::~MySerialPort()
{
    m_serial_port->close();
    delete [] m_recv_buffer;
}

void MySerialPort::onReadEvent(const char *portName, unsigned int buffer_len)
{
    if(buffer_len > 0)
    {
        if(m_read_data_callback)
        {
            if(m_recv_buffer_size + buffer_len > 1024)
            {
                LogError("receive buffer overflow");
                return;
            }
            m_serial_port->readData(m_recv_buffer + m_recv_buffer_size, buffer_len);
            m_recv_buffer_size += buffer_len;
            m_read_data_callback(m_recv_buffer, m_recv_buffer_size);
        }
    }
}

void MySerialPort::write(const char *data, size_t len)
{
    m_serial_port->writeData(data, len);
}

void MySerialPort::close()
{
    m_serial_port->close();
}

void MySerialPort::clear()
{
    m_recv_buffer_size = 0;
}