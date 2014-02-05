#include "Port.h"
#include "Node.h"
#include <sys/socket.h>
#include <sys/signalfd.h>
#include <cstring>
#include <unistd.h>
#include <ncurses.h>

using namespace std;

Port::Port(Node &node, unsigned id, ConnectionType connType) 
    : m_node(node), m_id(id), m_connType(connType), m_thread(0), m_sigFD(0), 
      m_kill(false), m_close(false), m_timeout(1), m_socket(0), 
      m_clientSocket(0), m_port(0), m_clientPort(0), 
      m_displayInfo(node.getDisplay()), m_connected(false)
{
    m_displayInfo.setID(to_string(id));
    m_displayInfo.setAddress("?");
    m_displayInfo.setPort("?");
    m_displayInfo.setStatus(ConnectionInfo::Status::NOT_CONNECTED);
    m_displayInfo.setClientID("?");
    m_displayInfo.setClientAddress("?");
    m_displayInfo.setClientPort("?");
}

Port::~Port()
{

}

void Port::start()
{
    void *port = this;
    pthread_create(&m_thread, nullptr, &Port::threadStart, port);
}

void* Port::threadStart(void *port)
{
    Port *p = reinterpret_cast<Port*>(port);
    p->run();
    return nullptr;
}

void Port::run()
{
    m_kill = false;
    string err;

    // Setup signal for receiving close and kill commands from console
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);

    m_sigFD = signalfd(-1, &sigset, 0);
    if (m_sigFD == -1) {
        unixError("Signal fd error");
        m_node.removePort(this);
        return;
    }

    // Add port to UI
    m_node.getDisplay().addConnection(&m_displayInfo);
    m_node.getDisplay().queueUpdate();

    while(!m_kill) {
        initialize();
        mainLoop();
        cleanup();

        // 10 sec timeout interrupted by SIGUSR1
        if(m_close && !m_kill) {
            fd_set readSet;
            FD_ZERO(&readSet);
            FD_SET(m_sigFD, &readSet);

            timeval tm;
            tm.tv_sec = m_timeout;
            tm.tv_usec = 0;

            int result = select(m_sigFD + 1, &readSet, nullptr, nullptr, &tm);
            
            if (result == -1) {
                unixError("Select error");
                return;
            }

            if (FD_ISSET(m_sigFD, &readSet)) {
                signalfd_siginfo si;
                if (read(m_sigFD, &si, sizeof(si)) != sizeof(si)) {
                    unixError("Signal read error");
                    return;
                }
            }
            m_timeout = 1;
        }
    }

    // Remove port from UI
    m_node.getDisplay().rmConnection(&m_displayInfo);
    m_node.getDisplay().queueUpdate();

    // Destroy port
    m_node.removePort(this);
}

void Port::portMsg(const std::string &msg) {
    string m("[");
    m += to_string(m_id);
    m += "|";
    m += m_IP;
    m += ":";
    m += to_string(m_port);
    m += " -> ";
    m += m_clientIP;
    m += ":";
    m += to_string(m_clientPort);
    m += "] ";
    m += msg;
    m_node.getDisplay().setError(m);
    m_node.getDisplay().queueUpdate();
}

void Port::unixError(const std::string &msg) {
    string  m(msg);
    m += ": ";
    m += strerror(errno);
    portMsg(m);
    m_close = true;
}

void Port::initialize()
{
    // Mark as not closed
    m_close = false;

    // Create socket
    m_socket = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_IP);
    if (m_socket == -1) {
        unixError("Cannot create a socket");
        return;
    }

    // Construct local address structure
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    m_addr.sin_port = htons(m_port);

    // Bind it to local address and port
    if (bind(m_socket, reinterpret_cast<sockaddr*>(&m_addr),
             sizeof(m_addr)) == -1) {
        unixError("Cannot bind a socket");
        return;
    }

    socklen_t len = sizeof(m_addr);
    if (getsockname(m_socket, 
                    reinterpret_cast<sockaddr*>(&m_addr), &len) == -1) {
        unixError("Cannot get socket name");
        return;
    }

    // Update IP/Port values
    m_port = ntohs(m_addr.sin_port);
    m_IP = inet_ntoa(m_addr.sin_addr);

    // Depends on type do listening or connection
    if (m_connType == ConnectionType::SERVER) {
        if (listen(m_socket, 1) == -1) {
            unixError("Listen error");
            return;
        }
        m_clientIP = inet_ntoa(m_clientAddr.sin_addr);
        m_clientPort = ntohs(m_clientAddr.sin_port);
    } else {
        // Setup address for connection
        memset(&m_clientAddr, 0, sizeof(m_clientAddr));
        m_clientAddr.sin_family = AF_INET;
        m_clientAddr.sin_addr.s_addr = inet_addr(m_clientIP.c_str());
        m_clientAddr.sin_port = htons(m_clientPort);

        // Update information
        m_displayInfo.setClientPort(to_string(m_clientPort));
        m_displayInfo.setClientAddress(m_clientIP);
    }

    // Update information
    m_displayInfo.setPort(to_string(m_port));
    m_displayInfo.setAddress(m_IP);
    m_node.getDisplay().queueUpdate();

    // Do listening/connect on select
    while(true) {
        int result;
       
        if (m_connType == ConnectionType::SERVER)
            result = accept(m_socket, 
                            reinterpret_cast<sockaddr*>(&m_clientAddr),
                            &len);
        else
            result = connect(m_socket, 
                             reinterpret_cast<const sockaddr*>(&m_clientAddr),
                             sizeof(m_clientAddr));

        // Check value
        if (m_connType == ConnectionType::SERVER) {
            if (result == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
                unixError("Accept error");
                return;
            }
            if (result >= 0) {
                // Close server socket and use connected socket
                ::close(m_socket);
                m_socket = result;

                // Get peer address and update UI
                m_clientPort = ntohs(m_clientAddr.sin_port);
                m_clientIP = inet_ntoa(m_clientAddr.sin_addr);
                m_displayInfo.setClientPort(to_string(m_clientPort));
                m_displayInfo.setClientAddress(m_clientIP);
                m_node.getDisplay().queueUpdate();

                break;
            }
        } else {
            if (result == -1 && errno != EINPROGRESS && errno != EALREADY) {
                unixError("Connection error");
                return;
            }
            if (result == 0) break;
        }

        // Do select
        fd_set readSet;
        FD_ZERO(&readSet);
        FD_SET(m_socket, &readSet);
        FD_SET(m_sigFD, &readSet);

        fd_set writeSet;
        FD_ZERO(&writeSet);
        FD_SET(m_socket, &writeSet);
        FD_SET(m_sigFD, &writeSet);

        result = select((m_socket > m_sigFD ? m_socket : m_sigFD) + 1,
                        &readSet, &writeSet, nullptr, nullptr);
        
        if (result == -1) {
            unixError("Select error");
            return;
        }

        if (FD_ISSET(m_sigFD, &readSet)) {
            signalfd_siginfo si;
            if (read(m_sigFD, &si, sizeof(si)) != sizeof(si)) {
                unixError("Signal read error");
                return;
            }
            unixError("Not expected");
            break;
        }
    }

    // If not closed, port has been connected
    if (!m_close && !m_kill) {
        m_connected = true;
        markClosed();
        connected();
    }
}

void Port::cleanup()
{
    if(m_socket) {
        shutdown(m_socket, SHUT_RDWR);
        ::close(m_socket);
        m_socket = 0;
    }

    if(m_clientSocket) {
        shutdown(m_clientSocket, SHUT_RDWR);
        ::close(m_clientSocket);
        m_clientSocket = 0;
    }

    // Port has been disconnected
    m_connected = false;
    m_displayInfo.setStatus(ConnectionInfo::Status::NOT_CONNECTED);
    m_node.getDisplay().queueUpdate();
    disconnected();
}

void Port::mainLoop()
{
    while(!m_close && !m_kill) {
        fd_set set;
        FD_ZERO(&set);
        FD_SET(m_socket, &set);
        FD_SET(m_sigFD, &set);
        int result = select((m_socket > m_sigFD ? m_socket : m_sigFD) + 1,
                            &set, nullptr, nullptr, nullptr);
        
        if (result == -1) {
            unixError("Select error");
            return;
        }

        // Handle signal
        if (FD_ISSET(m_sigFD, &set)) {
            signalfd_siginfo si;
            if (read(m_sigFD, &si, sizeof(si)) != sizeof(si)) {
                unixError("Signal read error");
                return;
            }
            break;
        }

        // Handle message
        if (FD_ISSET(m_socket, &set)) {
            char buffer[256];
            ssize_t size = recv(m_socket, buffer, 255, 0);
            
            if (size == -1) {
                unixError("Socket read error");
                return;
            }

            if (size == 0) {
                m_close = true;
                return;
            }

            string msg;
            msg.assign(buffer, size);
            gotMsg(msg); 
        }
    }
}

void Port::sendMessage(const string &msg, bool force)
{
    // If connected and opened
    if (!m_connected) return;

    // If not forced and not opened, abort.
    if (!force && m_displayInfo.getStatus() != ConnectionInfo::Status::OPENED)
        return;

    ssize_t size = send(m_socket, msg.c_str(), msg.length(), MSG_NOSIGNAL);
    if (size == -1) unixError("Send error");
    if (size != msg.length()) unixError("Didn't send full message");
}

void Port::markOpened() {
    if (m_connected) {
        m_displayInfo.setStatus(ConnectionInfo::Status::OPENED);
        m_node.getDisplay().queueUpdate();
    }
}

void Port::markClosed() {
    if (m_connected) {
        m_displayInfo.setStatus(ConnectionInfo::Status::CLOSED);
        m_node.getDisplay().queueUpdate();
    }
}
