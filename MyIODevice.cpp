#include "MyIODevice.h"

MyIODevice::~MyIODevice()
{

}

void MyIODevice::setReadDataCallback(std::function<void(const char *, size_t)> callback)
{
    m_read_data_callback = callback;
}

void MyIODevice::setErrorCallback(std::function<void(const char *)> callback)
{
    m_error_callback = callback;
}
