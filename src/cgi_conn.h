/*************************************************************************
	> File Name: cgi_conn.h
	> Author:kayejohn 
	> Mail:1572831416 
	> Created Time: Tue 03 Apr 2018 07:13:28 AM PDT
 ************************************************************************/
#ifndef _CGI_CONN_H
#define _CGI_CONN_H
#include <iostream>
#include <string>
#include <string.h>
#include <algorithm>
#include <sys/socket.h>
#include <sys/types.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdio.h>
#include "comm.h"

using namespace std;
class cgi_conn
{
private:
    static const int BUFF_SIZE  = 1024;
    static int epollfd;
    int sockfd;
    struct sockaddr_in client_addr;
    char buff[BUFF_SIZE];
    int read_index;
public:
    cgi_conn(){}
    ~cgi_conn(){}
    void init(int _epollfd,int _sockfd,struct sockaddr_in  _client_addr)
    {
        epollfd = _epollfd;
        sockfd = _sockfd;
        memset(&client_addr,'\0',BUFF_SIZE);
        client_addr = _client_addr;
        memset(buff,'\0',BUFF_SIZE);
        read_index = 0;
    }
    void process();
};

int cgi_conn::epollfd = -1;

void cgi_conn::process()
{
    int index = 0;
    int ret = -1;
    while( 1 )
    {
        index = read_index;
        ret = recv(sockfd,buff,BUFF_SIZE-1-index,0);
        if( ret < 0 )
        {
            if( errno != EAGAIN )
            {
                removefd(epollfd,sockfd);
            }
        }
        else if ( 0 == ret )
        {
            removefd(epollfd,sockfd);
            break; 
        }
        else
        {
            read_index += ret;
            cout << "user content is :"<< buff<<endl;
            for(; index<read_index; index++)
            {
                if( index >= 1 && buff[index] == '\n' && buff[index-1] == '\r') 
                {
                    break;
                }
            }
            if( index == read_index)
            {
                continue;
            }
            //是减去10,而不是11,因为数组是从0开始的 
            buff[index-10]  = '\0';
            char file_name[255] = "cgi/";
            //获得请求行中的CGI文件，
            //由于不关注该服务器与浏览器的交互，
            //专注于服务器的性能，
            //所以只写了CGI程序以便于测试
            sprintf(file_name,"cgi/%s",buff+5);
            //cout <<"file name:"<< file_name<<endl ;
            //判断文件是否存在
            if( access(file_name,F_OK) ==-1 )
            {
                removefd(epollfd,sockfd);
                break;
            }
            ret = fork();
            if( ret < 0  )
            {
                removefd(epollfd,sockfd);
                break;
            }
            else if( ret == 0 )
            {
                //将CGI的程序的输出重新定向到sockfd中，关掉输出描述符，使得dup创建的文件描述符为1
                close(STDOUT_FILENO);
                dup(sockfd);
                execl(file_name,file_name,NULL);
                usleep(10);
                exit(0);
            }
            else
            {
                //一定要关闭父进进程中的文件描述符，否则会一会与客户端相连接，以至于子进程中的数据不能刷新到浏览器
                removefd(epollfd,sockfd);
                break;
            }
        }
    }
}
#endif
