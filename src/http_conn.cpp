/*************************************************************************
> File Name: http_conn.cpp
> Author:kayejohn 
> Mail:1572831416 
> Created Time: Sat 14 Apr 2018 07:08:41 PM PDT
************************************************************************/
#include "http_conn.h"
#include "comm.h"

const char * ok_200_title = "OK";
const char * error_400_title = "Bad Request";
const char * error_400_form = "Your request has bad syntax or is inherrbtlly to statify.\n";

const char * error_403_title = "Forbidden";
const char * error_403_form = "You do not have permission to get file from this server.\n";

const char * error_404_title = "Not Found";
const char * error_404_form = "The requested file was not found on this serrver.\n";

const char * error_500_title = "Internal Error";
const char * error_500_form = "There was an unusual problem serbing the requested file.\n";
const char *doc_root = "/var/www/html";
int http_conn::m_user_count = 0;
int http_conn::m_epollfd = -1;

void http_conn::init(int sockfd,const sockaddr_in  &addr)
{
    m_sockfd = sockfd;
    m_address = addr;

    //端口复用
    int reuse = 1;
    setsockopt(m_sockfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    addfd(m_epollfd,m_sockfd,true);
    m_user_count ++;

    init();
}

void http_conn::init()
{
    m_check_state = CHECK_STATE_REQUESTLINE;
    m_linger = false;

    m_method = GET;
    m_url = NULL;
    m_version = NULL;
    m_content_length = 0;
    m_host = NULL;
    m_start_line = 0;
    m_check_idx = 0;
    m_read_idx = 0;
    m_write_idx = 0;
    memset(m_read_buf,'\0',READ_BUFFER_SIZE);
    memset(m_write_buf,'\0',WRITE_BUFFER_SIZE);
    memset(m_read_buf,'\0',FILENAME_LEN);
}
void http_conn::close_conn(bool real_close )
{
    if( real_close && (m_sockfd != -1) )
    {
        removefd(m_epollfd,m_sockfd);
        m_sockfd = -1;
        m_user_count --;
    }
}

//该处理的入口函数，从这里开始处理所有的用户请求
void http_conn::process()
{
    //处理读的信息函数，我只需要根据返回值判断是否正确就可以
    HTTP_CODE read_ret = process_read();
    if( read_ret == NO_REQUEST )
    {
        //返回NO_REQUEST说明没有读取完全，继续读取
        //可能会有疑问，为什么这样的话就可以读取呢？思考
        modfd(m_epollfd,m_sockfd,EPOLLIN);
        return ;
    }
    //分析请求行成功，就可以根据请求要求来回复数据
    bool write_ret = process_write(read_ret);
    if( !write_ret  )
    {
        close_conn();
    }

    modfd(m_epollfd,m_sockfd,EPOLLOUT);
}


http_conn::HTTP_CODE http_conn::process_read()
{
    LINE_STATUS line_status = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char * text = NULL;

    //一行一行的解析http请求内容
    while( ((m_check_state == CHECK_STATE_CONTENT) && (line_status == LINE_OK ))||
           ((line_status = parse_line()) == LINE_OK) )
    {
        text = get_line();
        m_start_line = m_check_idx;
        std::cout << "get 1 http line:" << text << std::endl;
        switch( m_check_state )
        {
            case CHECK_STATE_REQUESTLINE:
            {
                ret = parse_request_line(text);
                if( ret ==  BAD_REQUEST )
                {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER:
            {
                ret = parse_headers(text);
                if( ret ==  BAD_REQUEST )
                {
                    return BAD_REQUEST;
                }
                else if( ret == GET_REQUEST )
                {
                    return do_request();
                }
                break;
            }    
            case CHECK_STATE_CONTENT:
            {
                ret = parse_content(text);
                if( ret == GET_REQUEST )
                {
                    return do_request();
                }
                line_status = LINE_OPEN;
                break;
            }
            default:
            {
                break;
            }
        }
    }
    return NO_REQUEST;
}

//分析请求行
http_conn::HTTP_CODE http_conn::parse_request_line(char *text)
{
    //获得url
    m_url = strpbrk(text," \t") ;//返回text中第一个在“ \t”中的字符位置
    if( !m_url )
    {
        return BAD_REQUEST;
    }
    *m_url++ = '\0';

    //判断为GET方法
    char *method  = text;
    if( strcasecmp(method,"GET") == 0 )
    {
        m_method = GET;
    }
    else
    {
        return BAD_REQUEST;
    }

    //strspn()函数返回m_url中连续为" \t"的个数
    m_url += strspn(m_url," \t");
    //获得版本号
    m_version = strpbrk(m_url," \t");
    if( !m_version )
    {
        return BAD_REQUEST;
    }

    *m_version++ = '\0';
    m_version += strspn(m_version," \t");
    if( strcasecmp(m_version,"HTTP/1.1") != 0  ||  strcasecmp(m_version,"HTTP/1.0") != 0)
    {
        return BAD_REQUEST;
    }

    if( strncasecmp(m_url,"http://",7) == 0 )
    {
        //获得文件名称
        m_url += 7;
        m_url = strchr(m_url,'/');
        //查找第一次出现的位置
    }

    if( !m_url || m_url[0] != '/' )
    {
        return BAD_REQUEST;
    }

    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;

}

http_conn::HTTP_CODE http_conn::parse_headers(char *text)
{
    if( text[0] == '\0' )
    {
        if( m_content_length != 0 )
        {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if( strncasecmp(text,"Connection:",11) == 0 )
    {
        text += 11;
        text += strspn(text," \t");
        if( strcasecmp(text,"keep-alive") == 0 )
        {
            m_linger = true;
        }
    }
    else if( strncasecmp( text,"Connection-length:",15) == 0 )
    {
        text += 15;
        text +=strspn(text," \t");
        m_content_length = (int)atol(text);
    }
    else if( strncasecmp(text,"Host:",5) == 0  )
    {
        text += 5;
        text += strspn(text," \t");
        m_host = text;
    }
    else
    {
        std::cout<< "unknow header:"<<text<<std::endl;
    }
}
//加代码
http_conn::HTTP_CODE http_conn::parse_content(char *text)
{
    if( m_read_idx >= (m_content_length + m_check_idx) )    
    {
        text[m_content_length]  = '\0';
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

//如果得到一个完整的正确的http请求时，就应该分析目标文件的属性，并且将文件映射到m_file_address处
http_conn::HTTP_CODE http_conn::do_request()
{
    strcpy(m_real_file,doc_root) ;
    int len = strlen(doc_root);
    strncpy(m_real_file+len,m_url,FILENAME_LEN-len-1);
    if( stat(m_real_file,&m_file_stat) < 0)
    {
        return NO_RESOURCE;
    }
    if( !( m_file_stat.st_mode & S_IROTH ) )
    {
        return FORBIDDEN_REQUEST;
    }
    if( S_ISDIR(m_file_stat.st_mode) )
    {
        return BAD_REQUEST;
    }

    int fd = open(m_real_file,O_RDONLY);
    if( fd == -1 )
    {
        std::cout  << "open file " << m_real_file << "faile"<<std::endl;
    }
    m_file_address = (char *) mmap(0,m_file_stat.st_size,PROT_READ,MAP_PRIVATE,fd,0);
    close(fd);
    return FILE_REQUEST;
}

http_conn::LINE_STATUS http_conn::parse_line()
{
    char temp;
    for(; m_check_idx < m_read_idx; ++m_check_idx)
    {
        temp = m_read_buf[m_check_idx];
        if( temp == '\r' )
        {
            if( m_check_idx + 1 == m_read_idx )
            {
                return LINE_OPEN;
            }
            else if( m_read_buf[m_read_idx+1] == '\n' )
            {
                m_read_buf[m_check_idx++] = '\0';
                m_read_buf[m_check_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if( temp == '\n' )
        {
            if( m_check_idx > 1 && m_read_buf[m_read_idx-1] == '\r' )
            {
                m_read_buf[m_check_idx-1] = '\0';
                m_read_buf[m_check_idx++] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN; 
}

bool http_conn::process_write(HTTP_CODE ret)
{
    switch( ret ) 
    {
        case INTERNAL_ERROR:
        {
            add_status_line(500,error_500_title);
            add_headers(strlen(error_500_form));
            if( !add_content(error_500_form) )
            {
                return false;
            }
            break;
        }
        case BAD_REQUEST:
        {
            add_status_line(400,error_400_title);
            add_headers(strlen(error_400_form));
            if( !add_content(error_400_form) )
            {
                return false;
            }
            break;
        }
        case NO_RESOURCE:
        {
            add_status_line(404,error_404_title);
            add_headers(strlen(error_404_form));
            if( !add_content(error_404_form) )
            {
                return false;
            }
            break;
        }
        case FORBIDDEN_REQUEST:
        {
            add_status_line(403,error_403_title);
            add_headers(strlen(error_403_form));
            if( !add_content(error_403_form) )
            {
                return false;
            }
            break;
        }
        case FILE_REQUEST:
        {
            add_status_line(403,error_403_title);
            if( m_file_stat.st_size !=0 )
            {
                add_headers(m_file_stat.st_size);
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_idx;
                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;
                return true;
            }
            else
            {
                const char *ok_string = "<html><body></body></html>";
                add_headers(strlen(ok_string));
                if( !add_content(ok_string) )
                {
                    return false;
                }
            }
            break;
        }
        default:
            return false;
    }
}

bool http_conn::read()
{
    if( m_read_idx >= READ_BUFFER_SIZE ) 
    {
        return false;
    }

    int bytes_read = 0;
    while( true )
    {
        bytes_read = recv(m_sockfd,m_read_buf+m_read_idx,READ_BUFFER_SIZE-m_read_idx-1,0);
        if( bytes_read == -1 )
        {
            if( errno == EAGAIN || errno == EWOULDBLOCK )
            {
                break;
            }
            return false;
        }
        else if( bytes_read == 0 )
        {
            return false;
        }

        m_read_idx += bytes_read;
    }
    return true;
}

bool http_conn::write()
{
    int temp = 0;
    int bytes_have_send = 0;
    int bytes_to_send = m_write_idx;
    if( bytes_to_send  == 0)
    {
        modfd(m_epollfd,m_sockfd,EPOLLIN);
        init();
        return true;
    }

    while( 1 )
    {
        temp = writev(m_sockfd,m_iv,m_iv_count);
        if( temp <= -1 )
        {
            if( errno == EAGAIN )
            {
                modfd(m_epollfd,m_sockfd,EPOLLOUT);
                return true;
            }
            unmap();
            return false;
        }

        bytes_to_send -=temp;
        bytes_have_send += temp;
        if( bytes_to_send <= bytes_have_send )
        {
            unmap();
            if( m_linger )
            {
                init();
                modfd(m_epollfd,m_sockfd,EPOLLIN);
                return true;
            }
            else
            {
                modfd(m_epollfd,m_sockfd,EPOLLIN);
                return false;
            }
        }
    }
}

//下面这一组函数用来process_write函数的调用填充http应答
void http_conn::unmap()
{
    if( m_file_address )
    {
        munmap(m_file_address,m_file_stat.st_size);
        m_file_address = NULL;
    }
}
bool http_conn::add_respond(const char *format,...)
{
    if( m_write_idx >= WRITE_BUFFER_SIZE )    
    {
        return false;
    }
    va_list arg_list;
    va_start(arg_list,format);
    int len = vsnprintf(m_write_buf+m_write_idx,WRITE_BUFFER_SIZE-1-m_write_idx,format,arg_list);
    if( len >= (WRITE_BUFFER_SIZE-1-m_write_idx) )
    {
        return false;
    }
    m_write_idx+=len;
    va_end(arg_list);
    return true;
}

bool http_conn::add_status_line(int status ,const char * title)
{
    return add_respond("%s %d %s\r\n","HTTP/1.1",status,title);
}
bool http_conn::add_headers(int content_length)
{
    add_content_length(content_length);
    add_linger();
    add_blank_line();
}
bool http_conn::add_content(const char * content)
{
    return add_respond("%s",content);
}

bool http_conn::add_content_length(int content_length)
{
    return add_respond("Content-length: %d\r\n",content_length);
}
bool http_conn::add_linger()
{
    return add_respond("Connection: %s\r\n",(m_linger == true)?"keep-alive":"close"); 
}
bool http_conn::add_blank_line()
{
    return add_respond("%s","\r\n");
} 
