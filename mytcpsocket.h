#ifndef MYTCPSOCKET_H
#define MYTCPSOCKET_H
#include <boost/asio.hpp>
#include <thread>
#include <mutex>
#include <stdint.h>
#include "MyIODevice.h"

class MyUdpSocket;

class MyTcpSocket : public MyIODevice
{

    friend class MyUdpSocket;
    typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;
    typedef boost::shared_ptr<boost::asio::ip::tcp::acceptor> acceptor_ptr;

private:
    explicit MyTcpSocket(socket_ptr sock_ptr, uint32_t read_buffer_size = 1024);
public:
    explicit MyTcpSocket(uint32_t read_buffer_size = 1024);
    virtual ~MyTcpSocket();
    void disconnectFromHost();
    bool connectToHost(const char *hostName, uint16_t port);
    bool bind(uint16_t port);
    void setReadBufferSize(uint32_t buf_size);

    const char *peerAddress() const;
    uint16_t peerPort() const;

    const char *localAddress() const;
    uint16_t localPort() const;

    void write(const char *data, size_t size) override;
    void close() override;
    void clear() override;
    void socketErrorOccurred(const char *error_msg);
    void setNewConnectionCallback(std::function<void(MyTcpSocket *, MyTcpSocket *)> callback);
    void setConnectedCallback(std::function<void(bool)> callback);
    void disconnectedFromHost();


private:
    void asyncConnectCallback(const std::error_code &ec);
    void asyncReadCallback(const std::error_code &ec, size_t size);
    void asyncWriteCallback(const std::error_code &ec, size_t size);
    void asyncAcceptCallback(socket_ptr sock,const std::error_code &ec);

private:
    socket_ptr m_asio_socket;
    acceptor_ptr m_asio_acceptor;
    char *m_recv_buffer;
    size_t m_recv_buffer_size;
    std::mutex m_socket_mutex;
    char *m_asio_read_buf;
    size_t m_read_buffer_size;
    std::function<void(MyTcpSocket *, MyTcpSocket *)> m_new_connection_callback;
    std::function<void(bool)> m_connected_callback;
private:
    class MyIOContext
    {
    private:
        MyIOContext(){}
        static boost::asio::io_context *io_context;
        static std::thread *io_thread;
        static std::mutex io_mutex;
    public:
        static boost::asio::io_context *getIOContext();
    };
};

#endif // MYTCPSOCKET_H
