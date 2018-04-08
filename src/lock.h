/*************************************************************************
	> File Name: lock.h
	> Author:kayejohn 
	> Mail:1572831416 
	> Created Time: Sat 07 Apr 2018 11:54:38 PM PDT
 ************************************************************************/

#ifndef _LOCK_H
#define _LOCK_H
#include <iostream>
#include <pthread.h>
#include <semaphore.h>
#include <exception>

//封装信号量的类
class Csem
{
private:
    sem_t m_sem;
public:
    Csem()
    {
        if( sem_init(&m_sem,0,0) != 0 )
        {
            throw std::exception();
        }
    }
    ~Csem()
    {
        sem_destroy(&m_sem);
    }

    //等待信号量
    bool wait()
    {
        return sem_wait(&m_sem) == 0;
    }
    //增加信号量
    bool post()
    {
        return sem_post(&m_sem) == 0;
    }
};


//互斥锁
class Clocker
{
private:
    pthread_mutex_t mutex;
public:
    Clocker()
    {
        if( pthread_mutex_init(&mutex,NULL) != 0 )
        {
            throw std::exception();
        }
    }
    //销毁互斥锁
    ~Clocker()
    {
        pthread_mutex_destroy(&mutex);
    }

    //加锁
    bool lock()
    {
        return pthread_mutex_lock(&mutex) == 0;
    }
    //解锁
    bool unlock()
    {
        return pthread_mutex_unlock(&mutex) == 0;
    }
};

class Ccond
{
private:
    pthread_mutex_t mutex;
    pthread_cond_t cond;
public:
    Ccond()
    {
        if( pthread_mutex_init(&mutex,NULL) != 0 )
        {
            throw std::exception();
        }
        if( pthread_cond_init(&cond,NULL) != 0 )
        {
            pthread_mutex_destroy(&mutex);
            throw std::exception();
        }
    }
    ~Ccond()
    {
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }
    
    bool wait()
    {
        int ret = 0;
        pthread_mutex_lock(&mutex);
        ret = pthread_cond_wait(&cond,&mutex);
        pthread_mutex_unlock(&mutex);
        
        return ret == 0;
    }
    bool signal()
    {
        return pthread_cond_signal(&cond) == 0;
    }
};
#endif
