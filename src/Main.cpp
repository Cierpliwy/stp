#include "Display.h"
#include "Bridge.h"
#include "Client.h"
#include <string>
#include <unistd.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <ncurses.h>
#include <deque>
#include <fstream>
#include <iostream>
using namespace std;

int main(int argc, char *argv[])
{
    // Only argument is file for commands history
    deque<string> history;
    if (argc == 2) {
        //Open file and read all command line by line
        ifstream file(argv[1]);
        while (file.good()) {
            char buffer[256];
            file.getline(buffer, 255);
            history.push_back(buffer); 
        } 
    }

    Display display;
    display.setTitle(" EMPTY NODE - SR 2014 - by Przemysław Lenart");
    Node *node = nullptr;

    // Create file descriptor from signal
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGUSR1);
    sigaddset(&sigset, SIGWINCH);

    if (sigprocmask(SIG_BLOCK, &sigset, nullptr) == -1) return 1;
    int drawSignal = signalfd(-1, &sigset, 0);
    if (drawSignal == -1) return 1;

    bool exit = false; 
    string in;
    while(!exit) {
        display.update();
        // If there are no commands in history buffer
        if (history.empty()) {
            // Select used for input / drawing events
            fd_set set;
            FD_ZERO(&set);
            FD_SET(STDIN_FILENO, &set);
            FD_SET(drawSignal, &set);
            if (select(drawSignal + 1, &set, nullptr, nullptr, nullptr) == -1)
                return 1;

            // If it was only signal for drawing don't parse input
            if (FD_ISSET(drawSignal, &set)) {
                signalfd_siginfo si;
                if (read(drawSignal, &si, sizeof(si)) != sizeof(si)) return 1;
                if (si.ssi_signo == SIGWINCH) {
                    endwin();
                    refresh();
                }
                continue;
            }

            // Get characters
            int c = getch();
            if (c != '\n') {
                if (isprint(c))
                    in += c;
                if (c == 8 || c == 127 || c == 263)
                    if(!in.empty()) in.pop_back();

                display.setCmd(in);
                continue;
            }
        } else {
            // Get command from history
            in = history.front();
            history.pop_front();
        }

        // Clear last error
        if (!display.getError().empty()) display.setError("");

        if (in.compare("h") == 0)
            display.setDisplayHelp(true);
        else
            display.setDisplayHelp(false);

        if (in.compare("exit") == 0 || in.compare("quit") == 0 ||
            in.compare("q") == 0) {
            exit = true;
        }

        if (node)
            node->handleCommand(in);
        else {
            // Check if at least there are two strings
            auto pos = in.find_first_of(" ");
            if (pos != string::npos && pos+1 < in.length()) {
                auto c = in.substr(0, pos);
                bool valid = true;
                unsigned val;
                try {
                    val = stoul(in.substr(pos+1));
                } catch (exception &e) {
                    valid = false;
                }

                if (valid) {
                    if (c.compare("client") == 0)
                        node = new Client(val, display);
                    if (c.compare("bridge") == 0)
                        node = new Bridge(val, display);
                }
            }
        }

        in.clear();
        display.setCmd(in);
    }
    
    if (node) delete node;

    return 0;
}
