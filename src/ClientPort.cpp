#include "ClientPort.h"
#include "Client.h"

void ClientPort::gotMsg(const std::string &msg)
{
    // Extract client message
    unsigned int msgSize = 10;
    bool finished = false;
    m_buffer += msg;

    // Display raw message
    MsgInfo info;
    info.msg = msg;
    info.header = "RAW:";
    m_displayInfo.addMsg(info); 
    m_node.getDisplay().queueUpdate();

    // Get client msg
    while(!finished) {
        finished = true;
        for (int i = 0; i < m_buffer.size(); ++i) {
            if (m_buffer[i] == 'C' && i+msgSize-1 < m_buffer.size()) {
                messageReceived(m_buffer.substr(i, msgSize));
                finished = false;
                m_buffer = m_buffer.substr(i+msgSize);
                break;
            }
        }
    }
}

void ClientPort::messageReceived(const std::string &msg)
{
    uint32_t source;
    uint32_t dest;
    source = ntohl(*reinterpret_cast<const uint32_t*>(msg.c_str()+1));
    dest = ntohl(*reinterpret_cast<const uint32_t*>(msg.c_str()+5));
    char letter = msg.c_str()[9];

    // Check if message is for us
    if (dest == m_node.getID()) {
        // Save message
        MsgInfo info;
        info.header = "Got: '";
        info.header += letter;
        info.header += "' from: ";
        info.header += std::to_string(source);
        m_displayInfo.addMsg(info); 
        m_node.getDisplay().queueUpdate();
    }
}

void ClientPort::connected()
{
    markOpened();
}
