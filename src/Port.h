#ifndef PORT_H
#define PORT_H
#include "Display.h"
#include <arpa/inet.h>
#include <pthread.h>

class Node;

class Port
{
public:
    enum class ConnectionType {
        SERVER,
        CLIENT
    };

    Port(Node &node, unsigned id, ConnectionType connType);
    virtual ~Port();

    void start();

    void join() {
        pthread_join(m_thread, nullptr);
    }

    void kill() {
        m_kill = true;
        pthread_kill(m_thread, SIGUSR1);
    }

    void close(unsigned int seconds) {
        m_close = true;
        m_timeout = seconds;
        pthread_kill(m_thread, SIGUSR1);
    }

    int getID() const {
        return m_id;
    }

    void setPort(unsigned port) {
        m_port = port;
    }
    void setClientIP(const std::string &ip) {
        m_clientIP = ip;
    }
    void setClientPort(unsigned port) {
        m_clientPort = port;
    }

    // Mark socket as open
    void markOpened();

    // Mark socket as closed
    void markClosed();

    // Queue message to be send by this port
    void sendMessage(const std::string &msg, bool force);

    // Handle this function to process data
    virtual void gotMsg(const std::string &) {}

    // Port has been connected
    virtual void connected() {}

    // Port had been disconnected
    virtual void disconnected() {}

protected:

    static void* threadStart(void *port);
    void run();
    void initialize();
    void cleanup();
    void mainLoop();
    void portMsg(const std::string &msg);
    void unixError(const std::string &msg);

    Node &m_node;
    unsigned m_id;
    ConnectionType m_connType;

    pthread_t m_thread;
    int m_sigFD;
    bool m_kill;
    bool m_close;
    unsigned m_timeout;

    int m_socket;
    int m_clientSocket;

    sockaddr_in m_addr;
    sockaddr_in m_clientAddr;

    std::string m_IP;
    std::string m_clientIP;

    unsigned m_port;
    unsigned m_clientPort;

    ConnectionInfo m_displayInfo;
    bool m_connected;
};

#endif //PORT_H
