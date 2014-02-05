#include "BridgePort.h"
#include "Bridge.h"
using namespace std;

void BridgePort::gotMsg(const std::string &msg)
{
    // Extract bridge message
    unsigned int msgSize = 10;
    bool finished = false;
    m_buffer += msg;

    // Display raw message
    MsgInfo info;
    info.msg = msg;
    info.header = "RAW:";
    m_displayInfo.addMsg(info); 
    m_node.getDisplay().queueUpdate();

    // Get bridge msg
    while(!finished) {
        finished = true;
        for (int i = 0; i < m_buffer.size(); ++i) {
            if (m_buffer[i] == 'B' && i+msgSize-1 < m_buffer.size()) {
                bridgeMessageReceived(m_buffer.substr(i, msgSize));
                finished = false;
                m_buffer = m_buffer.substr(i+msgSize);
                break;
            }

            if (m_buffer[i] == 'C' && i+msgSize-1 < m_buffer.size()) {
                clientMessageReceived(m_buffer.substr(i, msgSize));
                finished = false;
                m_buffer = m_buffer.substr(i+msgSize);
                break;
            }
        }
    }
}

void BridgePort::bridgeMessageReceived(const std::string &msg)
{
    // Ignore if client port
    if (m_type == Type::CLIENT) return;

    // Get root id and root path
    uint32_t rootID = 
        ntohl(*reinterpret_cast<const uint32_t*>(msg.c_str() + 2));
    uint32_t rootPath = 
        ntohl(*reinterpret_cast<const uint32_t*>(msg.c_str() + 6));

    // Display message
    MsgInfo info;

    // Got message to open port
    if (msg[1] == (char)1) {
        markOpened();
        info.header = "Open port";
        m_displayInfo.addMsg(info); 
        m_node.getDisplay().queueUpdate();
        return;
    } else {
        info.header = "Root msg: root=";
        info.header += to_string(rootID);
        info.header += ", path=";
        info.header += to_string(rootPath);
        m_displayInfo.addMsg(info); 
        m_node.getDisplay().queueUpdate();
    }

    // Depending on state behaviour is different
    if (m_bridge.getState() == Bridge::State::WORKING) {
        info.header = "Initialize!";
        m_displayInfo.addMsg(info); 
        m_node.getDisplay().queueUpdate();

        // New bridge in network
        m_bridge.initialize();
        // Handle message
        bridgeRootMsg(rootID, rootPath);         
    }

    if (m_bridge.getState() == Bridge::State::LINKING) {
        // Handle message
        bridgeRootMsg(rootID, rootPath); 
    }
}

void BridgePort::clientMessageReceived(const std::string &msg)
{
    // Resend only if port is not blocked and in working state
    if (m_displayInfo.getStatus() == ConnectionInfo::Status::OPENED &&
        m_bridge.getState() == Bridge::State::WORKING)
        m_bridge.sendToOtherPorts(this, msg, false, true);
}

void BridgePort::connected()
{
    if (m_type == Type::CLIENT) {
        markOpened();
        return;
    }

    // Display message
    MsgInfo info;
    info.header = "Initialize!";
    m_displayInfo.addMsg(info); 
    m_node.getDisplay().queueUpdate();

    m_bridge.initialize();
}

void BridgePort::disconnected()
{
    if (m_type == Type::CLIENT) return;
    m_bridge.disconnected(this);
}

void BridgePort::bridgeRootMsg(unsigned rootID, unsigned rootPath)
{
    if (rootID < m_rootID || (rootID == m_rootID && rootPath < m_rootPath))
    {
        m_rootPath = rootPath;
        m_rootID = rootID;

        updatePortLabel();

        m_bridge.rootMsg(this, m_rootID, m_rootPath);
    }
}

void BridgePort::updatePortLabel()
{
    if (m_type == Type::CLIENT) {
        m_displayInfo.setClientID("client");
        m_node.getDisplay().queueUpdate();
    } else {
        string str = "root=";
        str += to_string(m_rootID);
        str += ", path=";
        str += to_string(m_rootPath);

        m_displayInfo.setClientID(str);
        m_node.getDisplay().queueUpdate();
    }
}
