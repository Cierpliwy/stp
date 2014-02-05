#include "Bridge.h"
#include "BridgePort.h"
#include <boost/regex.hpp>
#include <sys/signalfd.h>
using namespace std;

Bridge::Bridge(unsigned id, Display &display) 
    : Node(id, display), m_rootID(id), m_rootPath(0), m_state(State::WORKING),
      m_kill(false)
{
    pthread_mutexattr_init(&m_monitorAttr);
    pthread_mutexattr_settype(&m_monitorAttr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&m_monitor, &m_monitorAttr);

    pthread_create(&m_thread, nullptr, &Bridge::threadStart, this);

    updateTitle();
}

void* Bridge::threadStart(void *bridge)
{
    Bridge *b = reinterpret_cast<Bridge*>(bridge);
    b->start();
    return nullptr;
}

Bridge::~Bridge() 
{
    pthread_mutex_destroy(&m_monitor);
    pthread_mutexattr_destroy(&m_monitorAttr);
   
    // Kill thread
    m_kill = true;
    pthread_kill(m_thread, SIGUSR1);

    // Join it
    pthread_join(m_thread, nullptr);
}

void Bridge::start()
{
    // Setup signal for receiving signals from bridge
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);

    int sigFD = signalfd(-1, &sigset, 0);
    if (sigFD == -1) return;

    bool setTimer = false;

    while(!m_kill) {
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(sigFD, &readSet);

        timeval tm;
        tm.tv_sec = 10;
        tm.tv_usec = 0;

        timeval *tmPtr = nullptr;
        if (setTimer) tmPtr = &tm;

        int result = select(sigFD + 1, &readSet, nullptr, nullptr, tmPtr);
            
        if (result == -1) {
            //TODO Handle
            return;
        }

        // Get command
        if (FD_ISSET(sigFD, &readSet)) {
            signalfd_siginfo si;
            if (read(sigFD, &si, sizeof(si)) != sizeof(si)) {
                //TODO Handle
                return;
            }
            setTimer = true;
        } else {
            // We got timeout
            setTimer = false;
            timeout();
        }
    }
}

void Bridge::setTimeout()
{
    BridgeMonitor m(m_monitor);
    pthread_kill(m_thread, SIGUSR1);
}

Bridge::State Bridge::getState()
{
    BridgeMonitor m(m_monitor);
    return m_state;
}

void Bridge::updateTitle()
{
    BridgeMonitor m(m_monitor);

    string title = " BRIDGE(";
    title += to_string(m_id);
    title += ") [root=";
    title += to_string(m_rootID);
    title += ", path=";
    title += to_string(m_rootPath);
    title += "] State: ";
    switch(m_state) {
        case State::LINKING:
            title += "Linking";
            break;
        case State::WORKING:
            title += "Working";
            break;
    }
    m_display.setTitle(title);
    m_display.queueUpdate();
}

void Bridge::handleCommand(const string &command)
{
    BridgeMonitor m(m_monitor);

    // Regular expression structure
    boost::cmatch cm;
    boost::regex r;

    // Port creation
    if (command.substr(0,5).compare("cport") == 0) {
        r.assign("cport ([0-9]+) ?([0-9]+)?");
        regex_match(command.c_str(), cm, r);
        if (cm[1].matched) {
            auto port = new BridgePort(*this, stoul(cm[1]),
                                       Port::ConnectionType::SERVER,
                                       BridgePort::Type::CLIENT);
            if (cm[2].matched)
                port->setPort(stoul(cm[2]));
            addPort(port);
        }
        return;
    }

    if (command.substr(0,5).compare("bport") == 0) {
        r.assign("bport ([0-9]+) ?([0-9]+)?");
        regex_match(command.c_str(), cm, r);
        if (cm[1].matched) {
            auto port = new BridgePort(*this, stoul(cm[1]),
                                       Port::ConnectionType::SERVER,
                                       BridgePort::Type::BRIDGE);
            if (cm[2].matched)
                port->setPort(stoul(cm[2]));
            addPort(port);
        }
        return;
    }

    if (command.substr(0,5).compare("cconn") == 0) {
        r.assign("cconn ([0-9]+) ([0-9.]+):([0-9]+) ?([0-9]+)?");
        regex_match(command.c_str(), cm, r);
        if (cm[1].matched && cm[2].matched && cm[3].matched) {
            auto port = new BridgePort(*this, stoul(cm[1]),
                                       Port::ConnectionType::CLIENT,
                                       BridgePort::Type::CLIENT);
            port->setClientIP(cm[2]);
            port->setClientPort(stoul(cm[3]));
            if (cm[4].matched)
                port->setPort(stoul(cm[4]));
            addPort(port);
        }
    }

    if (command.substr(0,5).compare("bconn") == 0) {
        r.assign("bconn ([0-9]+) ([0-9.]+):([0-9]+) ?([0-9]+)?");
        regex_match(command.c_str(), cm, r);
        if (cm[1].matched && cm[2].matched && cm[3].matched) {
            auto port = new BridgePort(*this, stoul(cm[1]),
                                       Port::ConnectionType::CLIENT,
                                       BridgePort::Type::BRIDGE);
            port->setClientIP(cm[2]);
            port->setClientPort(stoul(cm[3]));
            if (cm[4].matched)
                port->setPort(stoul(cm[4]));
            addPort(port);
        }
    }

    // Close port
    if (command.substr(0,5).compare("close") == 0) {
        r.assign("close ([0-9]+) ([0-9]+)");
        regex_match(command.c_str(), cm, r);
        if (cm[1].matched && cm[2].matched) {
            auto port = m_ports.find(stoul(cm[1]));
            if (port != m_ports.end()) {
                port->second->close(stoul(cm[2]));
            } else {
                m_display.setError("Cannot close non existing port");
                m_display.queueUpdate();
            }
        }
    }

    // Kill port
    if (command.substr(0,4).compare("kill") == 0) {
        r.assign("kill ([0-9]+)");
        regex_match(command.c_str(), cm, r);
        if (cm[1].matched) {
            auto port = m_ports.find(stoul(cm[1]));
            if (port != m_ports.end()) {
                port->second->kill();
            } else {
                m_display.setError("Cannot kill non existing port");
                m_display.queueUpdate();
            }
        }
    }
}

void Bridge::initialize() {
    BridgeMonitor m(m_monitor);

    // Reset statistics only in WORKING state
    if (m_state == State::WORKING) {
        m_rootPath = 0;
        m_rootID = m_id;
    }

    // Prepare message
    string msg = "B";
    msg += (char)(0);
    uint32_t rootID = htonl(m_rootID);
    uint32_t rootPath = htonl(m_rootPath + 1);
    msg.append(reinterpret_cast<char*>(&rootID), 4);
    msg.append(reinterpret_cast<char*>(&rootPath), 4);

    // Send over all bridge ports
    for(auto &port : m_ports) {
        BridgePort *bridgePort = static_cast<BridgePort*>(port.second);
        if (bridgePort->getType() == BridgePort::Type::CLIENT) {
            bridgePort->markOpened();
        } else { 
            if (m_state == State::WORKING) {
                bridgePort->setRootID(m_id);
                bridgePort->setRootPath(0);
            }
            bridgePort->markClosed();
            bridgePort->updatePortLabel();
            port.second->sendMessage(msg, true);
        }
    }

    //Set state to linking and reset timeout
    m_state = State::LINKING;
    updateTitle();
    setTimeout();
}

void Bridge::sendToOtherPorts(Port *senderPort, const std::string &msg,
                              bool force, bool clientPorts) {
    BridgeMonitor m(m_monitor);
    for(auto &port : m_ports) {
        BridgePort *bridgePort = static_cast<BridgePort*>(port.second);
        if (bridgePort->getType() == BridgePort::Type::CLIENT &&
            !clientPorts) continue;
        if (bridgePort != senderPort)
            port.second->sendMessage(msg, force);
    }
}

void Bridge::rootMsg(Port *senderPort, unsigned rootID, unsigned rootPath)
{
    BridgeMonitor m(m_monitor);

    if (rootID < m_rootID || (rootID == m_rootID && rootPath < m_rootPath)) {
        m_rootID = rootID;
        m_rootPath = rootPath;
        updateTitle();
    }

    // Prepare message
    string msg = "B";
    msg += (char)(0);
    uint32_t msgRootID = htonl(rootID);
    uint32_t msgRootPath = htonl(rootPath + 1);
    msg.append(reinterpret_cast<char*>(&msgRootID), 4);
    msg.append(reinterpret_cast<char*>(&msgRootPath), 4);

    sendToOtherPorts(senderPort, msg, true, false);
}

void Bridge::disconnected(BridgePort *senderPort)
{
    BridgeMonitor m(m_monitor);
    // For now if disconnected and it's root port reinitialize algorithm
    if (senderPort->getRootID() == m_rootID &&
        senderPort->getRootPath() == m_rootPath)
        initialize();
}

void Bridge::timeout()
{
    BridgeMonitor m(m_monitor);

    // Open all non-root, not active ports
    for(auto &port : m_ports) {
        BridgePort *bridgePort = static_cast<BridgePort*>(port.second);
        if (bridgePort->getType() == BridgePort::Type::CLIENT) continue;

        if (bridgePort->getRootID() != m_rootID)
            bridgePort->markOpened();
    }

    // Open one root port
    for(auto &port : m_ports) {
        BridgePort *bridgePort = static_cast<BridgePort*>(port.second);
        if (bridgePort->getType() == BridgePort::Type::CLIENT) continue;

        if (bridgePort->getRootID() == m_rootID &&
            bridgePort->getRootPath() == m_rootPath &&
            m_rootID != m_id) {
            bridgePort->markOpened();

            // Send open port request message
            string msg = "B";
            msg += (char)(1);
            msg.append(8,(char)0);
            bridgePort->sendMessage(msg, true);

            break;
        }
    }

    m_state = State::WORKING;
    updateTitle();
}
