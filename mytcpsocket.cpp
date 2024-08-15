#include "mytcpsocket.h"
#include <boost/make_shared.hpp>
#include "utils.h"

using namespace boost::asio;

io_context *MyTcpSocket::MyIOContext::io_context = nullptr;
std::thread *MyTcpSocket::MyIOContext::io_thread = nullptr;
std::mutex MyTcpSocket::MyIOContext::io_mutex;

MyTcpSocket::MyTcpSocket(socket_ptr sock_ptr, uint32_t read_buffer_size)
{
    m_asio_socket  = sock_ptr;
    m_asio_acceptor = boost::make_shared<ip::tcp::acceptor>(*MyIOContext::getIOContext());
    m_asio_read_buf = nullptr;
    m_recv_buffer = nullptr;
    setReadBufferSize(read_buffer_size);
    m_asio_socket->async_read_some(buffer(m_asio_read_buf,m_read_buffer_size),std::bind(&MyTcpSocket::asyncReadCallback,this,std::placeholders::_1,std::placeholders::_2));
}

MyTcpSocket::MyTcpSocket(uint32_t read_buffer_size)
{
    m_asio_socket = boost::make_shared<ip::tcp::socket>(*MyIOContext::getIOContext());
    m_asio_acceptor = boost::make_shared<ip::tcp::acceptor>(*MyIOContext::getIOContext());
    m_asio_read_buf = nullptr;
    m_recv_buffer = nullptr;
    setReadBufferSize(read_buffer_size);
}

MyTcpSocket::~MyTcpSocket()
{
    std::unique_lock<std::mutex> lock(m_socket_mutex);
    m_error_callback = nullptr;
    try
    {
        m_asio_socket->cancel();
    }
    catch(boost::wrapexcept<boost::system::system_error> &error)
    {
        delete []m_asio_read_buf;
        delete []m_recv_buffer;
        return;
    }
    if(m_asio_socket->is_open())
    {
        try
        {
            m_asio_socket->shutdown(ip::tcp::socket::shutdown_type::shutdown_both);
        }
        catch (boost::wrapexcept<boost::system::system_error> error)
        {
            delete[] m_asio_read_buf;
            delete[] m_recv_buffer;
            return;
        }

        m_asio_socket->close();
    }
    if(m_asio_acceptor)
    {
        m_asio_acceptor->close();
    }
    delete[] m_asio_read_buf;
    delete[] m_recv_buffer;
}


void MyTcpSocket::close()
{
    std::unique_lock<std::mutex> lock(m_socket_mutex);
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
    if(m_asio_acceptor)
    {
        m_asio_acceptor->close();
    }

}

void MyTcpSocket::clear()
{
    m_recv_buffer_size = 0;
}

void MyTcpSocket::setNewConnectionCallback(std::function<void(MyTcpSocket *, MyTcpSocket *)> callback)
{
    m_new_connection_callback = callback;
}

void MyTcpSocket::setConnectedCallback(std::function<void(bool)> callback)
{
    m_connected_callback = callback;
}

void MyTcpSocket::asyncConnectCallback(const std::error_code &ec)
{
    if(m_connected_callback)
    {
        m_connected_callback(ec.value() == 0);
    }
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
        m_asio_socket->async_read_some(buffer(m_asio_read_buf,m_read_buffer_size),std::bind(&MyTcpSocket::asyncReadCallback,this,std::placeholders::_1,std::placeholders::_2));
    }

}


bool MyTcpSocket::bind(uint16_t port)
{

    std::unique_lock<std::mutex> lock(m_socket_mutex);
    try
    {
        m_asio_acceptor->open(ip::tcp::v4());
        m_asio_acceptor->set_option(ip::tcp::acceptor::reuse_address(true));
        m_asio_acceptor->bind(ip::tcp::endpoint(ip::tcp::v4(),port));
    }
    catch(boost::wrapexcept<boost::system::system_error> error)
    {
        if(m_error_callback)
        {
            m_error_callback(error.what());
        }
        return false;
    }
    m_asio_acceptor->listen(0);
    socket_ptr sock_(new ip::tcp::socket(*MyIOContext::getIOContext()));
    m_asio_acceptor->async_accept(*sock_,std::bind(&MyTcpSocket::asyncAcceptCallback,this,sock_,std::placeholders::_1));

    return true;
}

void MyTcpSocket::setReadBufferSize(uint32_t buf_size)
{

    std::unique_lock<std::mutex> lock(m_socket_mutex);
    m_read_buffer_size = buf_size;
    m_recv_buffer_size = 0;
    delete []m_asio_read_buf;
    delete []m_recv_buffer;
    m_asio_read_buf = new char[buf_size];
    m_recv_buffer = new char[buf_size];

}

const char *MyTcpSocket::peerAddress() const
{
    return m_asio_socket->remote_endpoint().address().to_string().data();
}

uint16_t MyTcpSocket::peerPort() const
{
    return m_asio_socket->remote_endpoint().port();
}

const char *MyTcpSocket::localAddress() const
{
    return m_asio_socket->local_endpoint().address().to_string().data();
}

uint16_t MyTcpSocket::localPort() const
{
    return m_asio_socket->local_endpoint().port();
}

void MyTcpSocket::write(const char *data, size_t size)
{
    std::unique_lock<std::mutex> lock(m_socket_mutex);
    m_asio_socket->async_write_some(buffer(data,size),std::bind(&MyTcpSocket::asyncWriteCallback,this,std::placeholders::_1,std::placeholders::_2));
}

bool MyTcpSocket::connectToHost(const char *hostName, uint16_t port)
{

    std::unique_lock<std::mutex> lock(m_socket_mutex);
    try
    {
        ip::tcp::endpoint host(ip::address::from_string(hostName),port);
        m_asio_socket->async_connect(host,std::bind(&MyTcpSocket::asyncConnectCallback,this,std::placeholders::_1));
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

void MyTcpSocket::disconnectFromHost()
{

    std::unique_lock<std::mutex> lock(m_socket_mutex);
    if(m_asio_socket->is_open())
    {
        m_asio_socket->shutdown(ip::tcp::socket::shutdown_type::shutdown_both);
        m_asio_socket->close();
    }
    
}


void MyTcpSocket::asyncReadCallback(const std::error_code &ec, size_t size)
{
    if(!ec)
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
            m_read_data_callback(m_recv_buffer,m_recv_buffer_size);
        }
        m_asio_socket->async_read_some(buffer(m_asio_read_buf,m_read_buffer_size),std::bind(&MyTcpSocket::asyncReadCallback,this,std::placeholders::_1,std::placeholders::_2));
    }
    else
    {
        if(m_error_callback)
        {
            m_error_callback(ec.message().c_str());
        }
    }

}

void MyTcpSocket::asyncWriteCallback(const std::error_code &ec, size_t size)
{
    if(ec)
    {
        if(m_error_callback)
        {
            m_error_callback(ec.message().c_str());
        }
    }
}

void MyTcpSocket::asyncAcceptCallback(socket_ptr sock,const std::error_code &ec)
{
    if(!ec)
    {
        std::unique_lock<std::mutex> lock(m_socket_mutex);
        MyTcpSocket *new_con = new MyTcpSocket(sock);
        if(m_new_connection_callback)
        {
            m_new_connection_callback(new_con, this);
        }
        socket_ptr sock_(new ip::tcp::socket(*MyIOContext::getIOContext()));
        m_asio_acceptor->async_accept(*sock_,std::bind(&MyTcpSocket::asyncAcceptCallback,this,sock_,std::placeholders::_1));
    }
    else
    {
        if(m_error_callback)
        {
            m_error_callback(ec.message().c_str());
        }
    }
}

boost::asio::io_context *MyTcpSocket::MyIOContext::getIOContext()
{
    if(io_context == nullptr)
    {
        io_mutex.lock();
        io_context = new boost::asio::io_context();
        io_thread = new std::thread([](){
            io_context::work worker(*io_context);
            io_context->run();
        });
        io_mutex.unlock();
    }
    return io_context;
}
