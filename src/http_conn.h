/*************************************************************************
	> File Name: http_conn.h
	> Author:kayejohn 
	> Mail:1572831416 
	> Created Time: Sat 14 Apr 2018 06:14:41 PM PDT
 ************************************************************************/
#ifndef _HTTP_CONN_H
#define _HTTP_CONN_H
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <iostream>
#include <sys/mman.h>
#include <stdarg.h>
#include <errno.h>

#include "lock.h"


//处理客户端的类
class http_conn
{
public:
    //文件名的最大长度
    static const int FILENAME_LEN = 200;
    //设置读缓冲区的大小
    static const int READ_BUFFER_SIZE = 2048;
    //设置写缓冲区的大小
    static const int WRITE_BUFFER_SIZE = 1024;
    //http请求方法
    enum METHOD {GET = 0,POST,HEAD,PUT,DELETE,TRACE,OPTIONS,PATCH};
    //解析客户请求时，主状态机所处的状态
    enum CHECK_STATE {CHECK_STATE_REQUESTLINE = 0,
                      CHECK_STATE_HEADER,
                      CHECK_STATE_CONTENT};
    //服务器处理请求后可能的结果
    enum HTTP_CODE{ NO_REQUEST,GET_REQUEST,BAD_REQUEST,
                    NO_RESOURCE,FORBIDDEN_REQUEST,FILE_REQUEST,
                    INTERNAL_ERROR,CLOSE_CONNECTION}; 
    //行的读取状态
    enum LINE_STATUS {LINE_OK = 0,LINE_BAD,LINE_OPEN};

public:
    http_conn(){}
    ~http_conn(){}

public:
    //初始化新接受的链接
    void init(int sockfd,const sockaddr_in  &addr);
    //关闭链接
    void close_conn(bool real_close  = true);
    //处理客户请求
    void process();
    //非阻塞读操作
    bool read();
    //非阻塞写操作
    bool write();

private:
    //初始化链接所需要的数据
    void init();
    //解析http请求
    HTTP_CODE process_read();
    //填充http应答
    bool process_write(HTTP_CODE ret);
    
    //用来分析http投不去所需要的信息
    HTTP_CODE parse_request_line(char *text);
    HTTP_CODE parse_headers(char *text);
    HTTP_CODE parse_content(char *text);
    HTTP_CODE do_request();
    char * get_line(){ return m_read_buf+m_start_line; };
    LINE_STATUS parse_line();

    //下面这一组函数用来process_write函数的调用填充http应答
    void unmap();
    bool add_respond(const char *format,...);
    bool add_content(const char * content);
    bool add_status_line(int status,const char * title);
    bool add_headers(int content_length);
    bool add_content_length(int content_length);
    bool add_linger();
    bool add_blank_line();
public:
    //所有的socket事件都注册到同一个epoll上，所以设置epoll为静态的
    static int m_epollfd;
    //统计用户量
    static int m_user_count;

private:
    int m_sockfd;
    sockaddr_in m_address;

    //读缓冲区
    char m_read_buf[READ_BUFFER_SIZE];
    //标志该缓冲区已经读入的客户数据最后一个字节的写一个位置
    int m_read_idx;
    //当前正在分析的字符在缓冲区的位置
    int m_check_idx;
    //当前正在解析行的起始位置
    int m_start_line ;

    //写缓冲区
    char m_write_buf[WRITE_BUFFER_SIZE];
    //写缓冲区的待发送的字节数
    int m_write_idx;

    //主状态机当前所处的状态
    CHECK_STATE m_check_state;
    METHOD m_method;

    //请求的目标文件完整路径
    char m_real_file[FILENAME_LEN];
    //请求目标文件的文件名
    char *m_url;
    char *m_version;
    char *m_host;
    int  m_content_length;
    bool m_linger;

    //客户端请求的文件被mmap到内存的位置
    char * m_file_address;
    //判断目标文件的状态，是否为目录，是否存在，是否可读
    struct stat m_file_stat;

    //由于使用writev函数写操作，定义内存块的结构与数量
    struct iovec m_iv[2];
    int m_iv_count;
};
#endif
