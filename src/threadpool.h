/*************************************************************************
	> File Name: threadpool.h
	> Author:kayejohn 
	> Mail:1572831416 
	> Created Time: Sat 07 Apr 2018 11:53:22 PM PDT
 ************************************************************************/

#ifndef _THREADPOOL_H
#define _THREADPOOL_H
#include <iostream>
#include <cstdio>
#include <pthread.h> 
#include <list>
#include "lock.h"

template<class T>
class threadpool
{
private:
    static const int MAX_PTHREAD_NUM = 50;
    static const int MAX_DEAL_TASK_NUM = 1000;
    
    //任务是队列，即链表
    std::list<T*> task_list;
    //锁,保护任务队列
    Clocker m_lock;
    //信号量,判断是否有任务需要处理
    Csem m_sem;
    //当前的线程个数
    int cur_pthread_num;
    //描述线程的数组
    pthread_t *pth_arr;

    //是否结束线程
    bool m_stop;
public:
    threadpool(int _cur_pthread_num = 8);
    ~threadpool();
    bool append_task(T * request);
private:
    //C++中使用pthread_create()函数时处理函数需要为静态成员函数
    static void *worker(void * arg);
    void run();
};

#endif
