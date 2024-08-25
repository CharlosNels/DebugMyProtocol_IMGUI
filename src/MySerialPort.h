#ifndef __MYSERIALPORT_H__
#define __MYSERIALPORT_H__

#include <CSerialPort/SerialPort.h>
#include <CSerialPort/SerialPortListener.h>
#include <cstddef>
#include "MyIODevice.h"

class MySerialPort : public itas109::CSerialPortListener , public MyIODevice
{
public:
    MySerialPort(itas109::CSerialPort *serial_port);
    virtual ~MySerialPort();
    void onReadEvent(const char *portName, unsigned int buffer_len) override;
    void write(const char *data, size_t len) override;
    void close() override;
    void clear() override;

private:
    itas109::CSerialPort *m_serial_port;
    char *m_recv_buffer;
    size_t m_recv_buffer_size;
};

#endif // __MYSERIALPORT_H__
