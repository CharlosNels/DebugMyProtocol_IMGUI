#ifndef MYIODEVICE_HPP
#define MYIODEVICE_HPP

#include <functional>

class MyIODevice {
  public:
    virtual ~MyIODevice();

    virtual void setReadDataCallback(std::function<void(const char *, size_t)> callback);

    virtual void write(const char *data, size_t size) = 0;

    virtual void close() = 0;

    virtual void setErrorCallback(std::function<void(const char *)> callback);

    virtual void clear() = 0;

  protected:
    std::function<void(const char *, size_t)> m_read_data_callback;
    std::function<void(const char *)> m_error_callback;
};

#endif
