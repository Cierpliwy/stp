#include "Client.h"
#include "ClientPort.h"
#include <boost/regex.hpp>
using namespace std;

Client::Client(unsigned id, Display &display) : Node(id, display), 
               m_port(nullptr)
{
    string title = " CLIENT (";
    title += to_string(id);
    title += ")";

    display.setTitle(title);
}

bool Client::addPort(Port *port) {
    bool res = Node::addPort(port);
    if (res) m_port = static_cast<ClientPort*>(port);
    return res;
}

bool Client::removePort(Port *port) {
    bool res = Node::removePort(port);
    if (res) m_port = nullptr;
    return res;
}

void Client::handleCommand(const string &command)
{
    // Regular expression structure
    boost::cmatch cm;
    boost::regex r;

    // Port creation
    if (command.substr(0,4).compare("port") == 0) {
        auto port = new ClientPort(*this, 0, Port::ConnectionType::SERVER);
        r.assign("port ([0-9]+)");
        regex_match(command.c_str(), cm, r);
        if (cm[1].matched) {
            unsigned portNr = stoul(cm[1]);
            port->setPort(portNr);
        }
        addPort(port);
        return;
    }

    // Close port
    if (command.substr(0,5).compare("close") == 0 && m_port) {
        r.assign("close ([0-9]+)");
        regex_match(command.c_str(), cm, r);
        if (cm[1].matched) {
            unsigned sec = stoul(cm[1]);
            m_port->close(sec);
        }
        return;
    }

    // Kill port
    if (command.compare("kill") == 0 && m_port) {
        m_port->kill();
        return;
    }

    // Connect to port
    if (command.substr(0,4).compare("conn") == 0) {
        r.assign("conn ([0-9.]+):([0-9]+) ?([0-9]+)?");
        regex_match(command.c_str(), cm, r);
        if (cm[1].matched && cm[2].matched) {
            unsigned portNr = stoul(cm[2]);
            auto port = new ClientPort(*this, 0, Port::ConnectionType::CLIENT);
            port->setClientIP(cm[1]);
            port->setClientPort(portNr);
            if (cm[3].matched) {
                portNr = stoul(cm[3]);
                port->setPort(portNr);
            }
            addPort(port);
        }
        return;
    } 

    //Send message
    if (command.substr(0,4).compare("send") == 0 && m_port) {
        r.assign("send ([0-9]+) ([[:alpha:]])");
        regex_match(command.c_str(), cm, r);
        if (cm[1].matched && cm[2].matched) {
            string msg = "C";
            uint32_t source = m_id;
            uint32_t dest = stoul(cm[1]);
            source = htonl(source);
            dest = htonl(dest);
            msg.append(reinterpret_cast<char*>(&source), 4);
            msg.append(reinterpret_cast<char*>(&dest), 4);
            msg.append(cm[2]);
            m_port->sendMessage(msg, true);
        }
        return;
    }
}
