#pragma once

#include <string>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#include <synapse/Event>

#include "./types.h"

//
#define BAUDRATE B9600

//
template<typename T>
class SerialReader
{
public:
    //
    SerialReader(int _argc, char *_argv[])
    {
        if (_argc < 2)
        {
            SYN_CORE_ERROR("usage: ", _argv[0], " <dev/interface> (e.g. ", _argv[0], " /dev/pts/3).")
            return;
        }

        // oper port
        m_fd = 0;
        m_fd = open(_argv[1], O_RDWR);
        if (m_fd == -1)
        {
            SYN_CORE_ERROR(strerror(errno));
            return;
        }
        m_devPath = std::string(_argv[1]);

        // serial port parameters
        struct termios newtio;
        memset(&newtio, 0, sizeof(newtio));
        struct termios oldtio;
        tcgetattr(m_fd, &oldtio);

        newtio = oldtio;
        newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
        newtio.c_iflag = 0;
        newtio.c_oflag = 0;
        newtio.c_lflag = 0;
        newtio.c_cc[VMIN] = 0;
        newtio.c_cc[VTIME] = 0;
        tcflush(m_fd, TCIFLUSH);

        cfsetispeed(&newtio, BAUDRATE);
        tcsetattr(m_fd, TCSANOW, &newtio);

        SYN_CORE_TRACE("opened serial connection on '", m_devPath, "'.");

    }
    
    //
    ~SerialReader()
    {
        close(m_fd);
    }

    //
    template<typename U>
    void read()
    {
        U val;
        int bytes = 0;
        
        bytes = ::read(m_fd, &val, sizeof(U));
        if (bytes == sizeof(U))
        {
            m_data.append(static_cast<T>(val));
            m_data.limits();
            m_updated = true;
        }

    }

    void reset()
    {
        m_updated = false;
    }

    // accessors
    auto &array() { return m_data; }
    T *data() { return m_data.data; }
    size_t dataSize() { return m_data.size; }
    size_t dataCapacity() { return m_data.capacity; }
    bool updated() { return m_updated; }


private:
    Array<T> m_data;
    int m_fd = 0;
    std::string m_devPath = "";
    bool m_updated = false;

};

