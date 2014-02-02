#ifndef NODE_H
#define NODE_H
#include "Display.h"
#include "Port.h"
#include <string>
#include <map>

class Node
{
public:

    Node(unsigned id, Display &display) : m_id(id), m_display(display) {}
    virtual ~Node() {}

    virtual void handleCommand(const std::string &command) = 0;

    virtual bool addPort(Port *port);
    virtual bool removePort(Port *port);

    Display& getDisplay() {
        return m_display;
    }
    
    unsigned getID() const {
        return m_id;
    }

protected:

    unsigned m_id;
    Display &m_display;
    std::map<unsigned, Port*> m_ports;
};

#endif //NODE_H
