#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include<sys/shm.h>
#include <string.h>
#include <string>
#include <list>
#include <random>
#include <iostream>
#include <thread>
#include <mutex>
#include <pthread.h>
#include <unistd.h>

using namespace std;

#define SHM_PIDLIST_KEY 0x1234
#define MAX_PROCESSES 10
#define MAX_MESSAGES 3
#define WRITER_DELAY 3
#define READER_DELAY 1
#define START_DELAY 2
#define DEBUG 0
int myPid;

struct msg_buffer {
    char msg[100];
    bool isNew = false;
    bool isFromManual = false;
    int from;
    int to;
    int which;

    void setContent(string content) {
        strcpy(msg, content.c_str());
    }
    int getWhich(){
        return which+1;
    }
};
struct node {
    int pid;
    bool isActive = false;
    bool isManual = false;
    int count = 0;
    msg_buffer messages[MAX_MESSAGES];

    bool isInboxFull() {
        return count == MAX_MESSAGES;
    }

    bool isReadyToGetMessage(bool excludeManual = true) {
        return pid != 0 && pid != myPid && isActive && count < MAX_MESSAGES && (excludeManual ? !isManual : true);
    }

    void addMessage(msg_buffer msg) {
        msg.which = count;
        messages[count++] = msg;
        if (count > MAX_MESSAGES) {
            isActive = false;
        }
    }

    bool hasUnreadMsg() {
        for (int x = 0; x < MAX_MESSAGES; x++) {
            if (messages[x].isNew) {
                return true;
            }
        }
        return false;
    }

    list<msg_buffer *> getUnreadMsg() {
        if (!hasUnreadMsg()) {
            throw runtime_error("Empty inbox");
        }
        list<msg_buffer *> filtered;
        for (int x = 0; x < MAX_MESSAGES; x++) {
            if (messages[x].isNew) {
                messages[x].isNew = false;
                filtered.push_back(&messages[x]);
            }
        }

        return filtered;
    }

    void printDebug() {
        cout << pid << " - " << isActive << " - " << count << " - " << isManual << "\n";
    }
};
struct pids {
public:
    int count = 0;
    node items[MAX_PROCESSES];

    list<node> getRecepients() {
        list<node> recepients = {};
        for (int x = 0; x < MAX_PROCESSES; x++) {
            if (DEBUG) items[x].printDebug();
            if (items[x].isReadyToGetMessage()) {
                recepients.push_front(items[x]);
            }
        }
        return recepients;
    }
    void printRecepients(bool excludeManual = true){
        list<node> recepients = getRecepients();
        string result;
        result.append("Possible recepients: (");
        for (int x = 0; x < MAX_PROCESSES; x++) {
            if (items[x].isReadyToGetMessage()) {
                result.append(to_string(items[x].pid));
                result.append(", ");
            }
        }
        result.append(")");
        cout << result << '\n';
    }
    void add(node node) {
        if (count > MAX_PROCESSES) {
            throw runtime_error("Cant add new node");
        }
        items[count++] = node;
    }

    node randomRecepient() {
        list<node> filtered;
        for (int x = 0; x < MAX_PROCESSES; x++) {
            if (items[x].isReadyToGetMessage()) {
                filtered.push_front(items[x]);
            }
        }
        std::random_device seed;
        // generator
        std::mt19937 engine(seed());
        // number distribution
        std::uniform_int_distribution<int> choose(0, filtered.size() - 1);
        // Create iterator pointing to first element
        auto it = filtered.begin();
        // Advance the iterator by 2 positions,
        advance(it, choose(engine));

        return *it;
    }

    node *getNode(int who) {
        for (int x = 0; x < MAX_PROCESSES; x++) {
            if (items[x].pid == who) {
                return &items[x];
            }
        }
        cout << "PROBLEMATIC " << who << "\n";
        throw runtime_error("Node not found");

    }
};
struct sharedMemory {
    pthread_mutexattr_t mattr;
    pthread_mutex_t multiProccessMutex;
    pids participients;
} *memory;

class Messanger {
public:
    pids *getParticipients() {
        return &memory->participients;
    }

    void lock() {
        pthread_mutex_lock(&memory->multiProccessMutex);
    }

    void unlock() {
        pthread_mutex_unlock(&memory->multiProccessMutex);
    }

    void addNode(node node) {
        getParticipients()->add(node);
    };

    node *getNode(int pid) {
        return getParticipients()->getNode(pid);
    }

    bool hasRecepients() {
        return !getParticipients()->getRecepients().empty();
    }

    void send(msg_buffer msg) {
        int recepient = msg.to;
        node *node = getParticipients()->getNode(recepient);
        node->addMessage(msg);
        cout << "PID: " << myPid << " SEND message to PID: " << msg.to << " with content: \"" << msg.msg << "\"\n";
    }

    bool hasMessages() {
        node *node = getParticipients()->getNode(myPid);
        return node->hasUnreadMsg();
    }

    list<msg_buffer *> inbox() {
        node *node = getParticipients()->getNode(myPid);
        list<msg_buffer *> msgs = node->getUnreadMsg();
        for (msg_buffer *msg: msgs) {
            msg->isNew = false;
        }

        return msgs;
    }
};

class App {
private:
    Messanger messanger;
public:
    void bot() {
        myPid = getpid();
        cout << "BOT PID: " << myPid << " STARTED\n";

        messanger.lock();
        node node;
        node.pid = myPid;
        node.isActive = true;
        messanger.addNode(node);
        messanger.unlock();

        sleep(START_DELAY);
    }

    void manual() {
        myPid = getpid();
        cout << "MANUAL PID: " << myPid << " STARTED\n";

        messanger.lock();
        node node;
        node.pid = myPid;
        node.isManual = true;
        messanger.addNode(node);
        messanger.unlock();
    }

    void botReader() {
        while (!isTimeToStopBot("READER")) {
            sleep(READER_DELAY);
            messanger.lock();

            if (messanger.hasMessages()) {
                list<msg_buffer *> inbox = messanger.inbox();
                for (msg_buffer *msg: inbox) {
                    cout << "PID: " << myPid << " RECEIVED " << msg->getWhich() << " from "<<msg->from<<" message with content: \"" << msg->msg<< "\"\n";
                    if (msg->isFromManual) {
                        int recepientPid = msg->from;
                        msg_buffer reply;
                        string replyContent =
                                "Hello manual. I'am node " + to_string(myPid) + ". Take a look on my stats. Got " +
                                to_string(msg->getWhich()) + " now";
                        reply.setContent(replyContent);
                        reply.from = myPid;
                        reply.to = recepientPid;
                        reply.isNew = true;

                        messanger.send(reply);
                    }
                }
            };

            messanger.unlock();
        }
        return;
    }

    void botWriter() {
        while (!isTimeToStopBot("WRITER")) {
            sleep(WRITER_DELAY);
            messanger.lock();
            memory->participients.printRecepients();
            if (messanger.hasRecepients()) {
                node recepient = memory->participients.randomRecepient();
                int recepientPid = recepient.pid;

                msg_buffer msg;
                string msgContent = "Hello node " + to_string(recepientPid) + ". I'am node " + to_string(myPid);
                msg.setContent(msgContent);
                msg.from = myPid;
                msg.to = recepientPid;
                msg.isNew = true;

                messanger.send(msg);
            }
            messanger.unlock();
        }
        return;
    }
    void manualWriter(){
        messanger.lock();
        string in;
        int recepientPid;
        cout << "Wpisz treść wiadomości: ";
        cin >> in;
        memory->participients.printRecepients();
        cout << "Wpisz PID odbiorcy: ";
        cin >> recepientPid;

        msg_buffer msg;
        string msgContent = "Hello node " + to_string(recepientPid) + ". I'am MANUAL " + to_string(myPid);
        msg.setContent(msgContent);
        msg.from = myPid;
        msg.to = recepientPid;
        msg.isNew = true;
        msg.isFromManual = true;

        messanger.send(msg);
        messanger.unlock();

        bool gotReply = false;

        while (!gotReply) {
            sleep(READER_DELAY);
            messanger.lock();

            if (messanger.hasMessages()) {
                list<msg_buffer *> inbox = messanger.inbox();
                for (msg_buffer *msg: inbox) {
                    cout << "PID: " << myPid << " RECEIVED " << msg->getWhich() << " from "<<msg->from<<" message with content: " << msg->msg<< "\n";
                }
                gotReply = true;
            };

            messanger.unlock();
        }

        return;
    }
private:
    bool isTimeToStopBot(string threadLabel) {
        node *me = messanger.getNode(myPid);
        bool isInboxFull = me->isInboxFull();
        if (isInboxFull) {
            cout << "PID: " << myPid << " [INBOX FULL] so ending " << threadLabel << "\n";
            return true;
        }
        bool isNoMoreRecepients = !messanger.hasRecepients();
        if (isNoMoreRecepients) {
            cout << "PID: " << myPid << " [NO MORE RECEPIENTS] so ending " << threadLabel << "\n";
            return true;
        }

        return false;
    }
};


int main(int argc, char *argv[]) {
    cout << "---------------------------------------------------------------------------------------\n";
    int shmid;
    shmid = shmget(SHM_PIDLIST_KEY, sizeof(memory), 0644 | IPC_CREAT);
    if (shmid == -1) {
        perror("Shared memory");
    }
    memory = static_cast<sharedMemory *>(shmat(shmid, 0, 0));
    pthread_mutexattr_init(&memory->mattr);
    pthread_mutexattr_setpshared(&memory->mattr, PTHREAD_PROCESS_SHARED);
    pthread_mutex_init(&memory->multiProccessMutex, &memory->mattr);

    App app;

    if (argc == 2) {
        app.manual();
        thread writer{&App::manualWriter, &app};
        writer.join();
        cout << "MANUAL: "<< getpid() <<" TERMINATE \n";
    } else {
        app.bot();
        thread writer{&App::botWriter, &app};
        thread reader{&App::botReader, &app};
        reader.join();
        writer.join();
        cout << "PID: " << getpid() << " TERMINATE \n";
    }
    return EXIT_SUCCESS;

}