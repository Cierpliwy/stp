#ifndef BRIDGE_H
#define BRIDGE_H
#include "Node.h"
#include <pthread.h>

class BridgePort;

class BridgeMonitor
{
public:
    BridgeMonitor(pthread_mutex_t &mutex) : m_mutex(&mutex) {
        pthread_mutex_lock(m_mutex);
    }
    ~BridgeMonitor() {
        pthread_mutex_unlock(m_mutex);
    }

private:
    pthread_mutex_t *m_mutex;
};

class Bridge : public Node
{
public:
    Bridge(unsigned id, Display &display);
    ~Bridge();
    void handleCommand(const std::string &command);

    void updateTitle();

    enum class State {
        LINKING,
        WORKING
    };

    State getState();

    // Protocol specific
    void initialize();
    void sendToOtherPorts(Port *senderPort, const std::string &msg, 
                          bool force, bool clientPorts);
    void rootMsg(Port *senderPort, unsigned rootID, unsigned rootPath);
    void disconnected(BridgePort *senderPort);
    void setTimeout();
    void timeout();

private:
    
    // Bridge thread (for timeouts)
    static void* threadStart(void *bridge);
    void start();

    unsigned m_rootID;
    unsigned m_rootPath;
    State m_state;

    pthread_t m_thread;
    bool m_kill;
    pthread_mutexattr_t m_monitorAttr;
    pthread_mutex_t m_monitor;
};

#endif //BRIDGE_H 
