/*************************************************************************
	> File Name: precesspool.h
	> Author:kayejohn 
	> Mail:1572831416 
	> Created Time: Tue 03 Apr 2018 07:11:44 AM PDT
 ************************************************************************/

#ifndef _PRECESSPOOL_H
#define _PRECESSPOOL_H
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include "comm.h"
class process
{
public:
    pid_t m_pid;
    int m_pipefd[2];
public:
    process():m_pid(-1){}
};

template <class T>
class processpool
{
private:
    //进程池允许的最大子进程数
    static const int MAX_PROCESS_NUMBER = 16;
    //每个进程最多能处理的客户数
    static const int MAX_DEAL_NUMBER  = 65535;
    static const int MAX_EVENT_NUMBER  = 10000;
    //进程池中当前的进程总数
    int m_process_number ;
    //子进程在进程在池中的序号
    int index ;
    //每个进程有一个epoll
    int epollfd;
    //监听socket
    int sockfd;
    //子进程通过m_stop来判断是否停止
    bool m_stop;
    //保存所有子进程的描述信息
    process *m_sub_process;
    //进程池的静态实例
    static processpool<T> * myprocesspool;
private:
    processpool(int listenfd,int process_number = 8);
public:
    static processpool<T>* create(int listenfd,int process_number = 8);
    ~processpool();
    void run();
private:
    //类的实例不需要调用这些函数，只是类内部使用，所以定义为私有函数
    //创建父子进程之间通信的管道
    void setup_sig_pipe();
    //运行父进程
    void run_parent();
    //运行子进程
    void run_child();
};

//每个进程自己的管道文件描述符，用于接受该子进程的处理信号

template <class T>
processpool<T> * processpool<T>::myprocesspool = NULL;

#endif
