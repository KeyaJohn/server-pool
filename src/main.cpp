/*************************************************************************
	> File Name: main.cpp
	> Author:kayejohn 
	> Mail:1572831416 
	> Created Time: Tue 03 Apr 2018 06:11:12 AM PDT
 ************************************************************************/
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "processpool.cpp"
#include "processpool.h"
#include "cgi_conn.h"

#define SERV_PORT 3344
#define SERV_IP "192.168.93.138"
#define LINK_NUM 10

using namespace std;

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

    ret = listen(sockfd,LINK_NUM);
    assert(ret != -1);
    
    //创建线程池，单例模式下的创建线程池，通过静态函数create()创建，传入监听文件描述符sockfd
    processpool<cgi_conn> *pool = processpool<cgi_conn>::create(sockfd);
    if( pool )
    {
        pool->run();
        delete pool;
    }
   
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
    close(sockfd);

    return 0;
}

