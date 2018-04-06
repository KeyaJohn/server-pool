/*************************************************************************
	> File Name: processpool.cpp
	> Author:kayejohn 
	> Mail:1572831416 
	> Created Time: Tue 03 Apr 2018 07:12:39 AM PDT
 ************************************************************************/
#include "processpool.h"
#include "comm.h"

using namespace std;
extern int sig_pipefd[2];

template <class T>
processpool<T>::processpool(int listenfd,int process_number):
    sockfd(listenfd),m_process_number(process_number),m_stop(false),index(-1)
{
    assert((process_number >> 0) && process_number <= MAX_PROCESS_NUMBER);
    
    //开辟process_number个子进程用于保存子进程的信息
    m_sub_process = new process[process_number];
    assert(m_sub_process != NULL);

    //开始使用fork创建子进程
    for(int i=0; i<process_number; i++)
    {
        int ret = socketpair(PF_UNIX,SOCK_STREAM,0,m_sub_process[i].m_pipefd);
        assert(0 == ret);

        m_sub_process[i].m_pid = fork();
        assert(m_sub_process[i].m_pid >= 0);

        if( m_sub_process[i].m_pid == 0 )//子进程
        {
            close( m_sub_process[i].m_pipefd[0]);
            index = i;
            //必须要break,这样才能使得子进程不创建子进程，因为还有循环呢
            break;
        }
        else//父进程
        {
            close( m_sub_process[i].m_pipefd[1]);
            //使用continue父进程继续创建子进程
            continue;
        }
    }
}

template <class T>
//静态函数在声明的时候需要static关键字，而定义的时候不需要加static
processpool<T>* processpool<T>::create(int listenfd,int process_number)
{
    if( !myprocesspool )    
    {
        myprocesspool = new processpool<T>(listenfd ,process_number);
        assert(myprocesspool != NULL);
    }
    return myprocesspool;
}

template <class T>
processpool<T>::~processpool()
{
    delete [] m_sub_process;
}


template <class T>
void processpool<T>::run()
{
    //如果此时的进程下标不为-1，说明进程不是父进程，则直接运行子进程
    if( index != -1 ) 
    {
        run_child();
        return ;
    }
    //只有主进程运行，且index为-1
    run_parent();
}


template <class T>
void processpool<T>::setup_sig_pipe()
{
    epollfd = epoll_create(MAX_DEAL_NUMBER);
    assert(epollfd != -1 );

    int ret = socketpair(PF_UNIX,SOCK_STREAM,0,sig_pipefd);
    assert(0 == ret);

    //将管道设置为非阻塞
    setnoblocking(sig_pipefd[1]);
    //将管道的另一端加入子进程epoll的监听队列中
    addfd(epollfd,sig_pipefd[0]);

    //设置信号处理函数 
    addsig(SIGCHLD,sig_handler);
    addsig(SIGTERM,sig_handler);
    addsig(SIGINT,sig_handler);
    addsig(SIGPIPE,SIG_IGN);
    
}

template <class T>
void processpool<T>::run_parent()
{
    setup_sig_pipe();

    addfd(epollfd,sockfd);

    epoll_event events[MAX_EVENT_NUMBER];

    int sub_process_counter = 0;
    int new_connfd = 1;
    int number = 0;
    int ret = -1;

    while( !m_stop )
    {
        number = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
        if( (number < 0) && (errno != EINTR))
        {
            cout << "epoll failure!"<<endl;
            break;
        }

        for(int i=0; i<number; i++)
        {
            int mysockfd = events[i].data.fd;
            if( mysockfd == sockfd )
            {
                int i = 0;
                while(  m_sub_process[sub_process_counter].m_pid == -1  )
                {
                    sub_process_counter++;
                    i++;
                    if( i==m_process_number )
                    {
                        break;
                    }
                }
                if( m_sub_process[sub_process_counter].m_pid == -1 )
                {
                    m_stop = true;
                    break;
                }
                send(m_sub_process[sub_process_counter].m_pipefd[0],(char*)&new_connfd,sizeof(new_connfd),0);
                cout << "send request to child:"<< sub_process_counter<< endl;
                //记住分配到的进程位置，以便下次从这里分配
                sub_process_counter = (++sub_process_counter) % m_process_number;
            }
            else if( (mysockfd == sig_pipefd[0]) && (events[i].events & EPOLLIN) )
            {
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0],signals,sizeof(signals),0);
                if( ret <= 0  )
                {
                    continue;
                }
                else
                {
                    for(int i=0; i<ret; i++)
                    {
                        switch(signals[i])
                        {
                            case SIGCHLD:
                            {
                                pid_t pid;
                                int stat;
                                while( (pid = waitpid(-1,&stat,WNOHANG) ) > 0)
                                {
                                    for( int i=0; i<m_process_number; i++ )
                                    {
                                        if( m_sub_process[i].m_pid == pid )
                                        {
                                            cout << "child :"<< pid << "exit"<<endl;
                                            close(m_sub_process[i].m_pipefd[0]);
                                            m_sub_process[i].m_pid = -1;
                                        }
                                    }
                                }

                                m_stop = true;
                                for(int i=0; i<m_process_number; i++)
                                {
                                    if( m_sub_process[i].m_pid != -1 )
                                    {
                                        m_stop = false;
                                    }
                                }
                                break;
                            }
                            case SIGTERM:
                            case SIGINT:
                            {
                                cout << "kill all the child "<<endl;
                                for(int i=0; i<m_process_number; i++)
                                {
                                    int pid = m_sub_process[i].m_pid;
                                    if( pid != -1 )
                                    {
                                        kill(pid,SIGTERM);
                                    }
                                }
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }
                    }
                }
            }
            else
            {
                continue;
            }
        }
    }
    close(epollfd);
}    

template <class T>
void processpool<T>::run_child()
{
    setup_sig_pipe();

    int pipefd= m_sub_process[index].m_pipefd[1];

    addfd(epollfd,pipefd);

    epoll_event events[MAX_EVENT_NUMBER];
    T*user = new T[MAX_DEAL_NUMBER];
    int sub_process_counter = 0;
    int number = 0;
    int ret = -1;
    
    while ( !m_stop )
    {
        number = epoll_wait(epollfd,events,MAX_EVENT_NUMBER,-1);
        if( (number < 0) && (errno != EINTR))
        {
            cout << "epoll failure!"<<endl;
            break;
        }

        for(int i=0; i<number; i++)
        {
            int fd = events[i].data.fd;
            
            if( fd == pipefd  && (events[i].events & EPOLLIN))
            {
                int client = 0;
                ret = recv(fd,(char*)&client,sizeof(client),0);
                cout << "收到从父进程传递的文件描述符"<< client<<endl;
                if( (ret < 0) && (errno != EAGAIN ) || ret == 0)
                {
                    continue; 
                }
                else
                {
                    struct sockaddr_in client_address;
                    socklen_t len = sizeof( client_address );
                    int connfd = accept(sockfd,(struct sockaddr*)&client_address,&len);
                    if( connfd < 0 )
                    {
                        cout << "child  errno is:"<< errno<<endl;
                        continue;
                    }
                    addfd(epollfd,connfd);
                    user[connfd].init(epollfd,connfd,client_address);
                }
            }
            else if( (fd == sig_pipefd[0]) && (events[i].events & EPOLLIN) )
            {
                int sig;
                char signals[1024];
                ret = recv(sig_pipefd[0],signals,sizeof(signals),0);
                if( ret <= 0  )
                {
                    continue;
                }
                else
                {
                    for(int i=0; i<ret; i++)
                    {
                        switch(signals[i])
                        {
                            case SIGCHLD:
                            {
                                pid_t pid;
                                int stat;
                                while( (pid = waitpid(-1,&stat,WNOHANG)) > 0)
                                {
                                    continue;
                                }
                                break;
                            }
                            case SIGTERM:
                            case SIGINT:
                            {
                                m_stop = true;
                                break;
                            }
                            default:
                            {
                                break;
                            }
                        }
                    }
                }
            }
            else if( events[i].events & EPOLLIN )
            {
                user[fd].process();
            }
            else
            {
                continue;
            }
        }
    }
    delete [] user;
    user = NULL;
    close(pipefd);
    close(epollfd);
}    

