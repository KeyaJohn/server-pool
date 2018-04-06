/*************************************************************************
	> File Name: comm.h
	> Author:kayejohn 
	> Mail:1572831416 
	> Created Time: Thu 05 Apr 2018 08:45:06 PM PDT
 ************************************************************************/

#ifndef _COMM_H
#define _COMM_H

extern int sig_pipefd[2];

static int setnoblocking(int fd)
{
    int old_option = fcntl(fd,F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd,F_SETFL,new_option);
    return old_option;
}

static void addfd(int epollfd,int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    epoll_ctl(epollfd,EPOLL_CTL_ADD,fd,&event);
    setnoblocking(fd);
}
static void removefd(int epollfd,int fd)
{
    epoll_ctl(epollfd,EPOLL_CTL_DEL,fd,0);
    close(fd);
}

static void sig_handler(int sig)
{
    int save_errno = errno;
    int msg = sig;
    //这里为什么只发送一个字节呢？信号不是int型变量吗？
    send(sig_pipefd[1],(char *)&msg,1,0);
    errno = save_errno;
}
static void addsig(int sig,void(handler)(int),bool restart = true)
{
    struct sigaction sa;
    memset(&sa,'\0',sizeof(sa));
    sa.sa_handler = handler;
    if( restart  )
    {
        sa.sa_flags |= SA_RESTART;
    } 
    sigfillset(&sa.sa_mask);
    assert(sigaction(sig,&sa,NULL)!= -1);
}

#endif
