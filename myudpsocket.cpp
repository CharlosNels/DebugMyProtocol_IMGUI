#include "myudpsocket.h"
#include "mytcpsocket.h"
#include <boost/make_shared.hpp>
#include <functional>
#include "utils.h"

using namespace boost::asio;

MyUdpSocket::MyUdpSocket(size_t read_buffer_size)
    : m_read_buffer_size(read_buffer_size)
{
    m_asio_socket = boost::make_shared<ip::udp::socket>(*MyTcpSocket::MyIOContext::getIOContext());
    m_asio_read_buf = nullptr;
    m_recv_buffer = nullptr;
    m_recv_buffer_size = 0;
    setReadBufferSize(read_buffer_size);
}

MyUdpSocket::~MyUdpSocket()
{
    m_error_callback = nullptr;
    close();
    delete []m_asio_read_buf;
    delete []m_recv_buffer;
}

void MyUdpSocket::setReadBufferSize(size_t buf_size)
{
    m_read_buffer_size = buf_size;
    delete []m_asio_read_buf;
    delete []m_recv_buffer;
    m_asio_read_buf = new char[buf_size];
    m_recv_buffer = new char[buf_size];
}

bool MyUdpSocket::connectTo(const char *host, uint16_t port)
{
    try{
        m_remote_ep = ip::udp::endpoint(ip::address::from_string(host), port);
        m_asio_socket->open(m_remote_ep.protocol());
        m_asio_socket->connect(m_remote_ep);
        m_asio_socket->async_receive_from(buffer(m_asio_read_buf,m_read_buffer_size), m_remote_ep, std::bind(&MyUdpSocket::asyncReceiveCallback,this,std::placeholders::_1,std::placeholders::_2));
    }
    catch(boost::wrapexcept<boost::system::system_error> error)
    {
        if(m_error_callback)
        {
            m_error_callback(error.what());
        }
        return false;
    }
    return true;
}

bool MyUdpSocket::bind(uint16_t port)
{
    try{
        m_local_ep = ip::udp::endpoint(ip::udp::v4(), port);
        m_asio_socket->open(m_local_ep.protocol());
        m_asio_socket->bind(m_local_ep);
        m_asio_socket->async_receive_from(buffer(m_asio_read_buf, m_read_buffer_size), m_remote_ep,std::bind(&MyUdpSocket::asyncReceiveCallback, this, std::placeholders::_1, std::placeholders::_2));
    }
    catch(boost::wrapexcept<boost::system::system_error> error)
    {
        if(m_error_callback)
        {
            m_error_callback(error.what());
        }
        return false;
    }
    return true;
}


void MyUdpSocket::close()
{
    std::unique_lock<std::mutex> lock(m_socket_mutex);
    m_asio_socket->cancel();
    if(m_asio_socket->is_open())
    {
        try
        {
            m_asio_socket->shutdown(ip::tcp::socket::shutdown_type::shutdown_both);
        }
        catch (boost::wrapexcept<boost::system::system_error> error)
        {
            if(m_error_callback)
            {
                m_error_callback(error.what());
            }
            return;
        }
        m_asio_socket->close();
    }
}

void MyUdpSocket::asyncSendCallback(const std::error_code &ec, size_t size)
{
    if(ec)
    {
        if(m_error_callback)
        {
            m_error_callback(ec.message().c_str());
        }
    }
}

void MyUdpSocket::asyncReceiveCallback(const std::error_code &ec, size_t size)
{
    if(ec)
    {
        if(m_error_callback)
        {
            m_error_callback(ec.message().c_str());
        }
    }
    else
    {   
        std::unique_lock<std::mutex> lock(m_socket_mutex);
        if(m_recv_buffer_size + size > m_read_buffer_size)
        {
            if(m_error_callback)
            {
                m_error_callback("receive buffer overflow");
            }
            return;
        }
        memcpy(m_recv_buffer + m_recv_buffer_size, m_asio_read_buf, size);
        m_recv_buffer_size += size;
        if(m_read_data_callback)
        {
            m_read_data_callback(m_recv_buffer, m_recv_buffer_size);
        }
        m_asio_socket->async_receive_from(buffer(m_asio_read_buf, m_read_buffer_size), m_remote_ep, std::bind(&MyUdpSocket::asyncReceiveCallback,this,std::placeholders::_1,std::placeholders::_2));
    }
}

void MyUdpSocket::write(const char *data, size_t size)
{
    m_asio_socket->async_send_to(buffer(data, size), m_remote_ep, std::bind(&MyUdpSocket::asyncSendCallback, this, std::placeholders::_1, std::placeholders::_2));
}

void MyUdpSocket::clear()
{
    m_recv_buffer_size = 0;
}
