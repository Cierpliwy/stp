#ifndef CLIENT_H
#define CLIENT_H
#include "Node.h"

class ClientPort;

class Client : public Node
{
public:
    Client(unsigned id, Display &display);
    void handleCommand(const std::string &command);
    bool addPort(Port *port);
    bool removePort(Port *port);

private:

    ClientPort *m_port;
};

#endif //CLIENT_H
