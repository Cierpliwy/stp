#include "Display.h"
#include <ncurses.h>
using namespace std;

Display::Display()
{
    setlocale(LC_ALL, "");
    initscr();

    start_color();
    use_default_colors();
    init_pair(1, -1, -1);
    init_pair(2, COLOR_GREEN, COLOR_GREEN);
    init_pair(3, COLOR_YELLOW, COLOR_YELLOW);
    init_pair(4, COLOR_RED, COLOR_RED);
    init_pair(5, COLOR_WHITE, COLOR_BLACK);
    init_pair(6, COLOR_BLUE, -1);
    init_pair(7, COLOR_RED, COLOR_BLACK);

    raw();
	keypad(stdscr, TRUE);
	noecho();
    m_help = true;

    m_drawingThread = pthread_self();
}

Display::~Display()
{
    endwin();
}

void Display::addConnection(const ConnectionInfo *conn)
{
    m_drawMutex.lock();
    m_conns.insert(conn);
    m_drawMutex.unlock();
}

void Display::rmConnection(const ConnectionInfo *conn)
{
    m_drawMutex.lock();
    auto it = m_conns.find(conn);
    if (it != m_conns.end())
        m_conns.erase(it);
    m_drawMutex.unlock();
}

void Display::queueUpdate()
{
    m_drawMutex.lock();
    pthread_kill(m_drawingThread, SIGUSR1);
    m_drawMutex.unlock();
}

void Display::update()
{
    m_drawMutex.lock();
    clear();

    // Calculate available dimensions
    unsigned y = getmaxy(stdscr);
    if (y < 3) return;

    attron(COLOR_PAIR(1));

    // Calculate space for each connection
    if (m_conns.size()) {
        unsigned connY = (y-2) / m_conns.size();
        unsigned currY = 1;
        
        // Draw each connection
        for(auto &conn : m_conns) {
            attron(A_BOLD);
            mvprintw(currY, 0, "[");
            switch(conn->getStatus()) {
                case ConnectionInfo::Status::NOT_CONNECTED:
                    attron(COLOR_PAIR(3));
                    printw("-");
                    break;
                case ConnectionInfo::Status::OPENED:
                    attron(COLOR_PAIR(2));
                    printw("o");
                    break;
                case ConnectionInfo::Status::CLOSED:
                    attron(COLOR_PAIR(4));
                    printw("c");
                    break;
            };
            attron(COLOR_PAIR(1));

            printw("] (%s) %s:%s -> (%s) %s:%s",
                    conn->getID().c_str(), conn->getAddress().c_str(), 
                    conn->getPort().c_str(), conn->getClientID().c_str(),
                    conn->getClientAddress().c_str(), 
                    conn->getClientPort().c_str());
            attroff(A_BOLD);

            // Draw possible msgs
            for (unsigned i = 0; i < conn->getMsgs().size() &&
                                 i < connY - 1; ++i)
            {
                auto &msg = conn->getMsgs()[conn->getMsgs().size() - 1 - i];
                mvprintw(currY + 1 + i, 0, " %s", msg.header.c_str());

                for(char c : msg.msg)
                    printw(" %.2X", c);
            }
                                     
            currY += connY;
        }
    }

    // Display help
    if (m_help) {

        m_helpLineOffset = 1;
        drawHelpLine();
        drawHelpLine(" * ","Startup commands:");

        drawHelpLine(" client <id>"," - create client node with specified id");

        drawHelpLine(" bridge <id>"," - create bridge node with specified id");
        drawHelpLine();

        drawHelpLine(" * ","Bridge commands:");

        drawHelpLine(" bport <id> [port_nr]"," - create bridge port with");
        drawHelpLine(""," specified id and start listening on it. You can");
        drawHelpLine(""," optionally specify port number.");

        drawHelpLine(" cport <id> [port_nr]"," - create client port with");
        drawHelpLine(""," specified id and start listening on it. You can");
        drawHelpLine(""," optionally specify port number.");

        drawHelpLine(" bconn <id> <ip>:<port> [port_nr]"," - create bridge");
        drawHelpLine(""," port with specified id and try to connect to");
        drawHelpLine(""," ip:address. You can optionally specify port");
        drawHelpLine(""," number.");

        drawHelpLine(" cconn <id> <ip>:<port> [port_nr]"," - create client");
        drawHelpLine(""," port with specified id and try to connect to");
        drawHelpLine(""," ip:address. You can optionally specify port");
        drawHelpLine(""," number.");

        drawHelpLine(" close <id> <seconds>"," - close port with specified");
        drawHelpLine(""," id for 'seconds' seconds.");

        drawHelpLine(" kill <id>"," - permanently close port with specified");
        drawHelpLine(""," id");
        drawHelpLine();

        drawHelpLine(" * ", "Client commands:");
        drawHelpLine(" port [port_nr]"," - open client port, you can ");
        drawHelpLine(""," optionally specify port number");

        drawHelpLine(" conn <ip>:<port_nr> [port_nr]"," - create client port");
        drawHelpLine(""," and connect it to <ip>:<port_nr> bridge/client.");
        drawHelpLine(""," You can specify your port number.");

        drawHelpLine(" send <id> <letter>"," - send letter to another client");
        drawHelpLine(""," with specified id.");

        drawHelpLine(" close <seconds>", " - close current port for 'second'");
        drawHelpLine(""," seconds.");
        drawHelpLine(" kill", " - kill current port");
        drawHelpLine();
        drawHelpLine(" * ", "Other commands:");
        drawHelpLine(" exit/quit/q", " - close program");
        drawHelpLine();
        drawHelpLine(" Type 'h' to see this message again...", "");
    }

    // Draw title and prompt
    attron(A_BOLD);
    attron(COLOR_PAIR(5));

    clearLine(0);
    mvprintw(0,0,"%s", m_title.c_str());

    attron(COLOR_PAIR(5));
    clearLine(y-1);

    if (!m_error.empty()) {
        attron(COLOR_PAIR(7));
        int pos = getmaxx(stdscr) - m_error.length();
        if (pos < 0) pos = 0;
        mvprintw(y-1,pos,"%s", m_error.c_str());
    }

    attron(COLOR_PAIR(5));
    mvprintw(y-1,0,"> %s", m_cmd.c_str());
    attroff(A_BOLD);
    refresh();

    m_drawMutex.unlock();
}

void Display::clearLine(int y)
{
    move(y,0);
    int x = getmaxx(stdscr);
    for(int i = 0; i < x; ++i) addch(' ');
}

void Display::drawHelpLine(const string &cmd, const string &desc)
{
    attron(COLOR_PAIR(1));
    clearLine(m_helpLineOffset);
    attron(COLOR_PAIR(6));
    attron(A_BOLD);
    mvprintw(m_helpLineOffset,0,"%s", cmd.c_str());
    attroff(A_BOLD);
    attron(COLOR_PAIR(1));
    printw("%s", desc.c_str());
    m_helpLineOffset++;
}
