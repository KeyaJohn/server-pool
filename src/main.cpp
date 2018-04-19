/*************************************************************************
	> File Name: main.cpp
	> Author:kayejohn 
	> Mail:1572831416 
	> Created Time: Tue 03 Apr 2018 06:11:12 AM PDT
 ************************************************************************/
#include "http_conn.h"
#include "comm.h"
#include "threadpool.cpp"
#define SERV_PORT 3344
#define SERV_IP "192.168.93.138"
#define MAX_FD 65535
#define MAX_EVENT_NUMBER 10000 


int sig_pipefd[2];

int main()
{
    int ret = -1;
    int sockfd = socket(AF_INET,SOCK_STREAM,0); 
    assert(sockfd >= 0);

    int flags = 1; 
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&flags,sizeof(flags));

    struct sockaddr_in serv_addr;
    memset(&serv_addr,'\0',sizeof(sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(SERV_IP);
    ret = bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    assert(ret != -1);
    
    ret = listen(sockfd,MAX_FD);
    assert(ret != -1);   


    addsig(SIGPIPE,SIG_IGN);
    
    //创建线程池
    threadpool<http_conn> * pool = NULL;
    try 
    {
        pool = new threadpool<http_conn>;
    }
    catch(...)
    {
        cout << "catch  \n";
        return 1;
    }
   
    //预先为每个可能的用户连接扥配一个http_conn对象
    http_conn * user = new http_conn[MAX_FD];
    int user_count = 0;

    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = epoll_create(5);
    assert(epollfd != -1);

    addfd(epollfd,sockfd,false);
    http_conn::m_epollfd = epollfd;

    while( true )
    {
        std::cout <<"wait epoll.......\n";
        int number = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
        if( number < 0  && errno != EAGAIN )
        {
            std::cout << "epoll failure\n";
            break;
        }
        for(int i=0; i<number; i++)
        {
            int fd = events[i].data.fd;
            if( sockfd == fd )
            {
                struct sockaddr_in client_address;
                socklen_t len = sizeof(struct sockaddr_in);
                int connfd = accept(sockfd,(struct sockaddr*)&client_address,&len);
                if( connfd < 0 )
                {
                    std::cout << "errno\n";
                    continue;
                }
                if( http_conn::m_user_count >= MAX_FD )
                {
                    std::cout <<"too mach\n";
                    continue;
                }
                cout << "获得新连接\n";
                user[connfd].init(connfd,client_address);
            }
            else if( events[i].events & (EPOLLRDHUP | EPOLLHUP|EPOLLERR) )
            {
                user[fd].close_conn();
            }
            else if( events[i].events & EPOLLIN )
            {
                if( user[fd].read())
                {
                    pool->append_task(user+fd);
                    std::cout << "append task succrssfully\n"<<std::endl;
                }
                else 
                {
                    user[fd].close_conn();
                }
            }
            else if( events[i].events & EPOLLOUT  )
            {
                std::cout<<"开始写入数据"<<std::endl;
                if( !user[fd].write() )
                {
                    user[fd].close_conn();
                }
            }
            else
            {
                
            }
        }
    }
    close(epollfd);
    close(sockfd);
    delete []user;
    delete pool;

    return 0;
}

/*
#include "processpool.cpp"
#include "cgi_conn.h"
int main()
{
    int ret = -1;
    int sockfd = socket(AF_INET,SOCK_STREAM,0); 
    assert(sockfd >= 0);

    int flags = 1; 
    setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&flags,sizeof(flags));

    struct sockaddr_in serv_addr;
    memset(&serv_addr,'\0',sizeof(sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = inet_addr(SERV_IP);
    ret = bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
    assert(ret != -1);

    ret = listen(sockfd,MAX_FD);
    assert(ret != -1);
    
    //创建线程池，单例模式下的创建线程池，通过静态函数create()创建，传入监听文件描述符sockfd
    processpool<cgi_conn> *pool = processpool<cgi_conn>::create(sockfd);
    if( pool )
    {
        pool->run();
        delete pool;
    }
*/   
/*  cout << "等待链接......"<<endl;
    struct sockaddr_in client;
    socklen_t len = sizeof(struct sockaddr_in);
    int clientfd = accept(sockfd,(struct sockaddr*)&client,&len);
    assert(clientfd != -1);
    
    while( 1 )
    {
        cgi_conn cgi;
        cgi.init(-1,clientfd,client);
        cgi.process();
    }
    */
 /*   close(sockfd);

    return 0;
}
*/






