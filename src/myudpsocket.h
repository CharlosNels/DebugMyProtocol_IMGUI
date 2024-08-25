#ifndef MYUDPSOCKET_H
#define MYUDPSOCKET_H

#include <boost/asio.hpp>
#include <mutex>
#include "MyIODevice.h"

class MyUdpSocket : public MyIODevice
{

    typedef boost::shared_ptr<boost::asio::ip::udp::socket> socket_ptr;
    typedef boost::shared_ptr<boost::asio::ip::udp::resolver> acceptor_ptr;

public:
    explicit MyUdpSocket(size_t read_buffer_size = 1024);
    ~MyUdpSocket();
    void setReadBufferSize(size_t buf_size);
    bool connectTo(const char *host, uint16_t port);
    bool bind(uint16_t port);
    void close() override;
    void write(const char *data, size_t size) override;
    void clear() override;

    // void socketErrorOccurred(const std::error_code &ec);

    // QIODevice interface

private:
    void asyncSendCallback(const std::error_code &ec, size_t size);
    void asyncReceiveCallback(const std::error_code &ec, size_t size);

protected:
    socket_ptr m_asio_socket;
    char *m_recv_buffer;
    size_t m_recv_buffer_size;
    std::mutex m_socket_mutex;
    char *m_asio_read_buf;
    size_t m_read_buffer_size;
    boost::asio::ip::udp::endpoint m_local_ep;
    boost::asio::ip::udp::endpoint m_remote_ep;
};

#endif // MYUDPSOCKET_H
