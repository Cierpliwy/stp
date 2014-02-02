#include "Node.h"
using namespace std;

bool Node::addPort(Port *port)
{
    auto it = m_ports.find(port->getID());
    if (it != m_ports.end()) {
        m_display.setError("Port already exists");
        m_display.queueUpdate();
        delete port;
        return false;
    }

    m_ports.insert(pair<unsigned, Port*>(port->getID(), port));
    port->start();
    return true;
}

bool Node::removePort(Port *port)
{
    auto it = m_ports.find(port->getID()); 
    if (it != m_ports.end()) {
        m_ports.erase(it);
        port->kill();
        port->join();
        delete port;
        return true;
    }
    delete port;
    return false;
}
