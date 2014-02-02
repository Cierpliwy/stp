#ifndef BRIDGE_PORT_H
#define BRIDGE_PORT_H
#include "Port.h"
#include "Bridge.h"

class BridgePort : public Port
{
public:
    
    enum class Type {
        CLIENT,
        BRIDGE
    };

    BridgePort(Node &node, unsigned id, Port::ConnectionType connType,
               Type type)
        : Port(node, id, connType), m_bridge(static_cast<Bridge&>(node)),
          m_rootID(0), m_rootPath(0), m_type(type) {}
    
    void gotMsg(const std::string &msg);
    void bridgeMessageReceived(const std::string &msg);
    void clientMessageReceived(const std::string &msg);
    void bridgeRootMsg(unsigned rootID, unsigned rootPath);

    void connected();
    void disconnected();

    void setRootID(unsigned rootID) {
        m_rootID = rootID;
    }
    unsigned getRootID() const {
        return m_rootID;
    }

    void setRootPath(unsigned rootPath) {
        m_rootPath = rootPath;
    }
    unsigned getRootPath() const {
        return m_rootPath;
    }

    Type getType() const {
        return m_type;
    }

    void updatePortLabel();

private:

    std::string m_buffer;
    Bridge &m_bridge;

    unsigned m_rootID;
    unsigned m_rootPath;

    Type m_type;
};

#endif //BRIDGE_PORT_H

