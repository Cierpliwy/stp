#ifndef DISPLAY_H
#define DISPLAY_H
#include <string>
#include <vector>
#include <set>
#include <mutex>
#include <signal.h>

class ConnectionInfo;

class Display
{
public:
    Display();
    ~Display();

    void addConnection(const ConnectionInfo* conn);
    void rmConnection(const ConnectionInfo* conn);

    std::mutex &getDrawMutex() {
        return m_drawMutex;
    }
   
    void setTitle(const std::string &title) {
        m_drawMutex.lock();
        m_title = title;
        m_drawMutex.unlock();
    }
    const std::string& getTitle() const {
        return m_title;
    }

    void setDisplayHelp(bool help) {
        m_drawMutex.lock();
        m_help = help;
        m_drawMutex.unlock();
    }
    bool isHelpDisplayed() const {
        return m_help;
    }

    void setError(const std::string &error) {
        m_drawMutex.lock();
        m_error = error;
        m_drawMutex.unlock();
    }
    const std::string& getError() const {
        return m_error;
    }

    void setCmd(const std::string& cmd) {
        m_cmd = cmd;
    }

    void update();
    void queueUpdate();

private:

    void clearLine(int x);
    void drawHelpLine(const std::string &cmd = "", 
                      const std::string &desc = "");

    unsigned m_helpLineOffset;
    std::mutex m_drawMutex;
    std::string m_cmd;
    std::string m_title;
    std::string m_error;
    std::set<const ConnectionInfo*> m_conns;
    bool m_help;
    pthread_t m_drawingThread;
};

struct MsgInfo {
    std::string header;
    std::string msg;
};

class ConnectionInfo
{
public:
    
    ConnectionInfo(Display &display) : m_display(display) {}

    enum class Status {
        NOT_CONNECTED,
        OPENED,
        CLOSED
    };

    void setAddress(const std::string &address) {
        m_display.getDrawMutex().lock();
        m_address = address;
        m_display.getDrawMutex().unlock();
    }
    const std::string& getAddress() const {
        return m_address;
    }

    void setClientAddress(const std::string &address) {
        m_display.getDrawMutex().lock();
        m_clientAddress = address;
        m_display.getDrawMutex().unlock();
    }
    const std::string& getClientAddress() const {
        return m_clientAddress;
    }

    void setPort(const std::string &port) {
        m_display.getDrawMutex().lock();
        m_port = port;
        m_display.getDrawMutex().unlock();
    }
    const std::string& getPort() const {
        return m_port;
    }

    void setClientPort(const std::string &port) {
        m_display.getDrawMutex().lock();
        m_clientPort = port;
        m_display.getDrawMutex().unlock();
    }
    const std::string& getClientPort() const {
        return m_clientPort;
    }

    void setID(const std::string &id) {
        m_display.getDrawMutex().lock();
        m_id = id;
        m_display.getDrawMutex().unlock();
    }
    const std::string& getID() const {
        return m_id;
    }

    void setClientID(const std::string &id) {
        m_display.getDrawMutex().lock();
        m_clientId = id;
        m_display.getDrawMutex().unlock();
    }
    const std::string& getClientID() const {
        return m_clientId;
    }

    void setStatus(Status status) {
        m_display.getDrawMutex().lock();
        m_status = status;
        m_display.getDrawMutex().unlock();
    }
    Status getStatus() const {
        return m_status;
    }

    void addMsg(const MsgInfo &msg) {
        m_display.getDrawMutex().lock();
        m_msgs.push_back(msg);
        m_display.getDrawMutex().unlock();
    }

    const std::vector<MsgInfo>& getMsgs() const {
        return m_msgs;
    }

private:

    Display &m_display;

    std::string m_address;
    std::string m_port;
    std::string m_id;

    std::string m_clientAddress;
    std::string m_clientPort;
    std::string m_clientId;

    Status m_status;
    std::vector<MsgInfo> m_msgs;
};

#endif
