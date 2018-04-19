/*************************************************************************
	> File Name: threadpool.cpp
	> Author:kayejohn 
	> Mail:1572831416 
	> Created Time: Sun 08 Apr 2018 12:58:03 AM PDT
 ************************************************************************/
#include <iostream>
#include <string>
#include <algorithm>
#include <pthread.h>
#include "threadpool.h"
using namespace std;

template<class T>
threadpool<T>::threadpool(int _cur_pthread_num):
    m_stop(false),
    cur_pthread_num(_cur_pthread_num),
    pth_arr(NULL)
{     
    if( cur_pthread_num <= 0 || cur_pthread_num > MAX_PTHREAD_NUM ) 
    {
        //throw std::exception();
        cout<<"线程数值不对!\n";
    }

    pth_arr = new pthread_t[cur_pthread_num];
    if( !pth_arr )
    {
        throw std::exception();
    }

    for(int i=0; i<cur_pthread_num; i++)
    {
        if( pthread_create(pth_arr+i,NULL,worker,(void *)this) != 0 )
        {
            delete []pth_arr;
            cout <<"创建线程失败\n";
            //throw std::exception();
            return ;
        }
        else
        {
            cout << "创建线程成功："<<i<<"th pthread\n";
        }
    }
}

template<class T>
threadpool<T>::~threadpool()
{
    delete []pth_arr;
    m_stop = true;
}
 
template<class T>
bool threadpool<T>::append_task( T * request )
{
    m_lock.lock();
    if( task_list.size() > MAX_DEAL_TASK_NUM )
    {
        m_lock.unlock() ;
        return false;
    }
    task_list.push_back(request);
    m_lock.unlock() ;
    m_sem.post();
    return true;
}

template<class T>
void *threadpool<T>::worker(void * arg)
{
    //脱离主线程，使得所有子线程由系统管理，释放资源
    pthread_detach(pthread_self());

    threadpool * pool = (threadpool*) arg;
    pool->run();
    return pool;
}

  
template<class T>
void threadpool<T>::run()
{
    while( !m_stop ) 
    {
        //如果没有任务，就等待阻塞
        m_sem.wait();
        m_lock.lock();
        //为什么类中没有std::list,此处显示错误
        if( task_list.empty() )
        {
            m_lock.unlock();
            continue;
        }
        //取出任务队列中的任务
        T * request = task_list.front();
        task_list.pop_front();
        m_lock.unlock();
        if( !request )
        {
            continue;
        }
        //进行处理
        std::cout << pthread_self()<<"---------------------\n" <<std::endl;
        request->process();
    }
    cout << pthread_self()<<"线程 退出\n";
}
