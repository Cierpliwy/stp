#ifndef CLIENT_PORT_H
#define CLIENT_PORT_H
#include "Port.h"

class ClientPort : public Port
{
public:
    ClientPort(Node &node, unsigned id, Port::ConnectionType connType)
        : Port(node, id, connType) {}
    
    void gotMsg(const std::string &msg);
    void messageReceived(const std::string &msg);

    void connected();

private:
    std::string m_buffer;
};

#endif //CLIENT_PORT_H
