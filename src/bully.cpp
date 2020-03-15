#include "bully.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <sys/un.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <vector>
#include <string>
#include <thread>
#include <list>
#include <condition_variable>
using namespace std;
char UN_PRE[] = "/tmp/bas/";

void rec(int server_sockfd, int _id)
{
    listen(server_sockfd, 10);
    int sockfd;
    socklen_t len;
    message msg;
    struct sockaddr_un address;
    char send_buf[128];
    len = sizeof address;

    while (true)
    {
        sockfd = accept(server_sockfd, (sockaddr *)&sockfd, &len);
        auto n = read(sockfd, &msg, sizeof(send_buf));
        if (n == 1)
        {
            write(sockfd, " ", 1);
        }
        else if (msg.type == VICTORY)
        {
            Leader = msg.id;
            cout << "RECV"
                 << " " << msg.id << " "
                 << "VICTORY" << endl;
            leader_cond_.notify_all();
        }
        else if (msg.type == ELECTION && _id > msg.id)
        {
            cout << "RECV"
                 << " " << msg.id << " "
                 << "ELECTION" << endl;
            msg = {_id, ALIVE};
            if(Leader = _id)
                leader_cond_.notify_all();
            write(sockfd, &msg, sizeof msg);
            cout << "SEND"
                 << " " << msg.id << " "
                 << "ALIVE" << endl;
        }
    }
}

void start_recv(int a, int b)
{
    thread t(rec, a, b);
    sleep(5);
    t.detach();
}

void init(int id)
{ 
    int server_sockfd;
    struct sockaddr_un server_address;

    server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0); //创建socket，指定通信协议为AF_UNIX,数据方式SOCK_STREAM
    //配置server_address
    auto un_path = UN_PRE + to_string(id);
    server_address.sun_family = AF_UNIX;
    strcpy(server_address.sun_path, un_path.c_str());
    unlink(server_address.sun_path);
    auto server_len = offsetof(sockaddr_un, sun_path) + strlen(server_address.sun_path);
   
    if(bind(server_sockfd, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        perror("bind:");
    }
    cout<<"It is ok"<<endl;
    start_recv(server_sockfd, id);
}

message _send(int id, message msg, bool reply)
{
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1)
        return msg;
    auto un_path = UN_PRE + to_string(id);
    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, un_path.c_str());
    if (connect(sockfd, (sockaddr *)&addr, sizeof(addr)) == -1)
    {
        std::string tmp = strerror(errno);
        // "CONN err";
        tmp.append(to_string(id));
        // log(tmp);
        goto end;
    }
    // log(SEND, msg.id, msg.type);
    if (write(sockfd, &msg, sizeof msg) == -1)
        goto end;
    // log(SEND, msg.id, msg.type);
    if (reply)
    {
        if (read(sockfd, &msg, sizeof msg) != -1)
            // log(RECV, msg.id, msg.type);
             cout << "RECV"
                 << " " << msg.id << " "
                 << "ALIVE" << endl;
        else
            goto end;
    }
    return msg;

    end:
        // log("ERR");
        cout<<"ERR"<<endl;
        close(sockfd);
        return msg;
}
int send_(int id,char *buf,size_t len)
{
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1)
        return -1;
    auto un_path = UN_PRE + to_string(id);
    sockaddr_un addr;
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, un_path.c_str());
    if (connect(sockfd, (sockaddr *)&addr, sizeof(addr)) == -1)
        return -1;
    if (write(sockfd, buf, len) == -1)
        return -1;
    return read(sockfd, buf, len);
}
void broadcastVictory(int a)
{
    std::list<int>::iterator it;
    for (it = idList.begin(); it != idList.end(); ++it)
    {
        if(*it != a)
            _send(*it, {a ,VICTORY}, false);
        else
            cout<<"VICTORY"<<endl;
    }
}

void selectLeader(int _id)
{
    std::list<int>::iterator it;
    int maxid = idList.back();
    if (_id == maxid)
    {
        Leader = _id;
        broadcastVictory(_id);
    }
    else
    {
        bool getRely = false;
        for (it = idList.begin(); it != idList.end(); it++)
        {
            if (_id < *it)
            {
                cout <<"SEND"<<" "<<*it<<" "<<"ELECTION"<<endl;
                message msg = _send(*it, {_id,ELECTION},true);
                if (msg.id == _id)
                {        
                }
                else
                {
                    if(msg.id > _id && msg.type == ALIVE)
                    {
                        getRely = true;
                        break;
                    }  
                }
                
            }
        }

        if (!getRely)
        {
            Leader = _id;
            broadcastVictory(_id);
        }
        else
        {
            std::unique_lock<std::mutex> lock{mutex_};
            if (leader_cond_.wait_for(lock, 3s, [&] () { return Leader != -1;}))
            {
                // ok
            }
            else
            {
                // timeout
                selectLeader(_id);
            }
        }
    }
}   
bool check_leader_alive()
{
    char buf = ' ';
    if(Leader != -1){
        return send_(Leader, &buf, 1) != -1;
    }
}
int main(int argc, char *argv[])
{
    auto id = stoi(argv[1]);
    init(id);
    this_thread::sleep_for(5s);
    selectLeader(id);
    while (true)
    {
        if (Leader == id)
        {
            std::unique_lock<std::mutex> lock{mutex_};
            leader_cond_.wait(lock);
        }
        else
        {
            this_thread::sleep_for(3s);
            if (!check_leader_alive())
            {
                selectLeader(id);
            }
        }
    }
    return 0;
}
