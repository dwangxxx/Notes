IO复用就是通过一个线程监听多个端口，可以用于以下场景：

> 客户端程序要同时处理多个socket。
> 客户端程序要同时处理用户输入和网络连接。
> TCP服务器要同时处理监听socket和连接socket。
> 服务器要同时处理TCP请求和UDP请求。
> 服务器要同时监听多个端口，或者处理多种服务。

## select系统调用

select系统调用的用途是在一段指定时间内，监听用户感兴趣的文件描述符上的可读、可写和异常事件。

- select API

```C++
#include <sys/select.h>
int select(int nfds, fd_set* readfds, fd_set* writefds, fd_set* exceptfds, struct timeval* timeout);
```

> (1) nfds参数指定被监听的文件描述符的总数。被设置为select监听的所有文件描述符中的最大值加1。

> (2) readfds、writefds和exceptfds参数分别指向可读、可写和异常事件对应的文件描述符集合，是一个fd_set结构体：
```C++
#include <typesizes.h>
#define __FD_SETSIZE 1024

#include <sys/select.h>
#define FD_SETSIZE __FD_SETSIZE
typedef long inr __fd_mask;
#undef __NFDBITS
#define __NFDBITS (8 * (int) sizeof (__fd_mask))
typedef struct
{
#ifdef __USE_XOPEN
    __fd_mask fds_bits[ __FD_SETSIZE / __NFDBITS ];
#define __FD_BITS(set) ((set)->fds_bits)
#else
    __fd_mask __fds_bits[ __FD_SETSIZE / __NFDBITS ];
#define __FDS_BITS(set) ((set)->__fds_bits);
#endif
} fd_set;
```
fd_set结构体仅包含一个整型数组，该数组的每个元素的每一位(bit)标记一个文件描述符。fd_set能容纳的文件描述符数量由FD_SETSIZE指定，这限制了select能同时处理的文件描述符的总量。使用下列的哄来访问fd_set结构体中的位：
```C++
#include <sys/select.h>
FD_ZERO(fd_set *fdset);                 // 清除fdset的所有位
FD_SET(int fd, fd_set *fdset);          // 设置fdset的位fd
FD_CLR(int fd, fd_set *fdset);          // 清除fdset的位fd
int FD_ISSET(int fd, fd_set *fdset);    // 测试fdset的位fd是否被设置
```

> (3) timeout参数用来设置select函数的超时时间。它是一个timeval结构类型的指针。
```C++
struct timeval
{
    long tv_sec;        // 秒数
    long tv_usec;       // 微秒数
}
```

下列情况下socket可读：
> socket内核接收缓冲区的字节数大于或等于其低水位标志SO_RCVLOWAT。此时可以无阻塞地读取该socket，并且读操作返回的字节数大于0。
> socket通信的对方关闭连接。此时对该socket的读操作将返回0。
> 监听socket上有新的连接请求。
> socket上有未处理的错误。

下列情况下socket可写：
> socket内核发送缓冲区的可用字节数大于等于其低水位标志SO_SNDLOWAT。此时可以无阻塞地写该socket，并且写操作返回的字节数大于0。
> socket的写操作被关闭，对写操作被关闭的socket执行写操作将触发 一个SIGPIPE信号。
> socket使用非阻塞connect连接成功或者失败之后。
> socket上有未处理的错误。

select能处理的异常情况只有一种：socket上接收到带外数据。

## poll系统调用

poll系统调用和select类似，也是在指定时间内轮询一定数量的文件描述符，以测试其中是否有就绪者。poll的原型：
```C++
#include <poll.h>
int poll(struct pollfd* fds, nfds_t nfds, int timeout);
```

> (1) fds参数是一个pollfd结构类型的数组，指定所有我们感兴趣的文件描述符上发生的可读、可写和异常等事件。pollfd结构体的定义如下：
```C++
struct pollfd
{
    int fd;         // 文件描述符
    short events;   // 注册的事件
    short revents;  // 实际发生的事件，由内核填充
};
```
其中，fd成员指定文件描述符；events成员告诉poll监听fd上的哪些事件，是一系列事件的按位与。nfds是文件描述符的总数。

- epoll调用

epoll使用一组函数来完成任务，而不是单个函数。其次，epoll把用户关心的文件描述符上的事件放在内核里的一个事件表中，从而无需像select和poll那样每次调用都要重复传入文件描述符集合事件集。但是epoll需要使用一个额外的文件描述符，来唯一标识内核中的这个事件表。这个文件描述符使用如下epoll_create函数来创建：
```C++
#include <sys/epoll.h>
int epoll_create(int size); // return epfd
```

函数返回的文件描述符将用作其他所有的epoll系统调用的第一个参数，以指定要访问的内核事件表。下面的函数用来操作epoll内核事件表：
```C++
#include <sys/epoll.h>
int epoll_ctl(int epfd, int op, int fd, struct epoll_event* event);
```

fd参数是要操作的文件描述符，op参数则指定操作类型。操作类型有如下三种：
> EPOLL_CTL_ADD, 往事件表中注册fd上的事件。
> EPOLL_CTL_MOD, 修改fd上的注册事件。
> EPOLL_CTL_DEL, 删除fd上的注册事件。

event参数指定事件，它是epoll_event结构指针类型。epoll_event定义如下：
```C++
struct epoll_event
{
    __uint32_t events;      // epoll事件, EPOLLIN, EPOLLOUT
    epoll_data_t data;      // 用户数据
};

typedef union epoll_data_t 
{
    void* ptr;
    int fd;         // 事件从属的目标文件描述符
    uint32_t u32;
    uint64_t u64;
}  epoll_data_t;
```

epoll系列系统调用的主要接口是epoll_wait函数。它在一段超时时间内等待一组文件描述符上的事件，原型如下：
```C++
#include <sys/epoll.h>
int epoll_wait(int epfd, struct epoll_event* events, int maxevents, int timeout);
```

epoll_wait函数如果检测到事件，就将所有就绪的事件从内核事件表(由epfd参数指定)中复制到它的第二个参数events指向的数组中。这个数组只用于输出epoll_wait检测到的就绪事件，而不像select和poll的数组参数那样既用于传入用户注册事件，又用于输出内核检测到的就绪事件。极大提高了应用程序索引就绪文件描述符的效率。
```C++
// poll和epoll在使用上的差别
// 如何索引poll返回的就绪文件描述符
int ret = poll(fds, MAX_EVENT_NUMBER, -1);
// 必须遍历所有已注册文件描述符并找到其中的就绪者
for (int i = 0; i < MAX_EVENT_NUMBER; ++i)
{
    if (fds[i].revents & POLLIN)
    {
        int sockfd = fds[i].fd;
        // 处理socket
    }
}

// 如何索引epoll返回的就绪文件描述符
int ret = epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
for (int i = 0; i < ret; i++)
{
    int sockfd = events[i].data.fd;
    // sockfd肯定就绪，直接处理
}
```

- LT模式和ET模式

epoll对文件描述符的操作有两种模式：LE(Level Trigger，电平触发)模式和ET(Edge Trigger，边沿触发)模式。LT模式是默认的工作模式。

对于采用LT工作模式的文件描述符，当epoll_wait检测到其上有事件发生并将此事件通知应用程序之后，应用程序可以不立即处理该事件。这样，当应用程序下一次调用epoll_wait时，epoll_wait还会再次向应用程序通知此事件，知道事件被处理。而对于采用ET工作模式的文件描述符，当epoll_wait检测到其上有事件发生并将此事件通知应用程序后，应用程序必须处理该事件，因为后续的epoll_wait调用将不再向应用程序通知这一事件。

LT模式触发条件：
> 当buffer中有数据可读的时候，即buffer不空的时候
> 当buffer由不可读状态变为可读状态的时候，即由空变为不空的时候
> 当有新数据到达时，即buffer中的待读内容变多的时候

ET模式触发条件：
> 当buffer由不可读状态变为可读状态的时候，即由空变为不空的时候

- EPOLLONESHOT事件

即使使用ET模式，一个socket上的某个事件还是可能被触发多次。这在并发程序中就会引起一个问题，比如一个线程在读取完某个socket上的数据后开始处理数据，而在数据处理过程中该socket上又有新数据可读(EPOLLIN再次被触发)，此时另外一个线程被唤醒来读取这些新的数据。于是就出现了两个线程同时操作一个socket的局面。我们期望一个socket连接在任意时刻只能被一个线程处理，可以使用EPOLLONESHOT事件实现。

对于注册了EPOLLONESHOT事件的文件描述符，操作系统最多触发其上注册的一个可读、可写或者异常事件，而且只触发一次。因此当某个注册了EPOLLONESHOT事件的socket一旦被某个线程处理完毕，该线程就应该立即重置这个socket上的EPOLLONESHOT事件，以确保这个socket下一次可读时，其EPOLLIN事件能被触发。EPOLLONESHOT事件使用例子：
```C++
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <pthread.h>

#define MAX_EVENT_NUMBER 1024
#define BUFFER_SIZE 1024
struct fds
{
    int epollfd;
    int sockfd;
};

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

// 将fd上的EPOLLIN和EPOLLET事件注册到epollfd指示的epoll内核事件表中，参数oneshot指定是否注册fd上的EPOLLONESHOT事件
void addfd(int epollfd, int fd, bool oneshot)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET;
    if (oneshot)
    {
        event.events |= EPOLLONESHOT;
    }

    epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    setnonbloccking(fd);
}

// 重置fd上的事件，这样操作以后，即使fd上的EPOLLONESHOT事件被注册，但是操作系统仍然能会触发fd上的EPOLLIN事件，而且只触发一次
void reset_oneshot(int epollfd, int fd)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, fd, &event);
}

// 工作线程
// 参数是一个结构体
void worker(void* arg)
{
    int sockfd = ((fds*) arg)->sockfd;
    int epollfd = ((fds*) arg)->epollfd;
    printf("start new thread to receive data on fd: %d\n", sockfd);
    char buf[BUFFER_SIZE];
    memset(buf, '\0', BUFFER_SIZE);
    // 循环读取sockfd上的数据，知道遇到EAGAIN错误
    while (1)
    {
        int ret = recv(sockfd, buf, BUFFER_SIZE - 1, 0);
        if (ret == 0)
        {
            close(sockfd);
            printf("foreiner closed the connection\n");
            break;
        }
        else if (ret < 0)
        {
            // 读到了缓冲区末尾，没有数据了
            if (errno == EAGAIN)
            {
                // 重置sockfd的EPOLLONESHOT标志，以便后面数据可以被触发
                reset_oneshot(epollfd, sockfd);
                printf("read later\n");
                break;
            }
        }
        else
        {
            printf("get content: %s\n", buf);
            sleep(5);
        }
    }
    printf("end thread receiving data on fdL: %d\n", sockfd);
}

int main(int arg, char* argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    // 字符串转整数
    int port = atoi(argv[2]);
    
    // IPv4地址定义
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    // 网络类型, AP_INTE(IPv4)
    address.sin_family = AF_INET;
    // 将点分IP转换为整数地址，并将其赋给address.sin_addr
    inet_pton(AF_INET, ip, &address.sin_addr);
    // 网络字节序转换
    address.sin_port = htons(port);

    // 监听端口
    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    // 绑定地址
    int ret = bind(listenfd, (struct sockaddr*) address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 5);
    assert(ret != -1);

    epoll_event events[MAX_EVENT_NUMBER];
    // 第一步：创建epoll文件描述符
    int epollfd = epoll_create(5);
    assert(epollfd != -1);

    // 注意：监听socket listenfd上是不能注册EPOLLONESHOT事件的，否则应用程序只能处理一个客户连接，因为后续的客户连接请求将不再触发listenfd上的EPOLLIN事件
    addfd(epollfd, listenfd, false);

    while (1)
    {
        int ret = epoll_wait(epollf, events, MAX_EVENT_NUMBER, -1);
        if (ret < 0)
        {
            printf("epoll failure\n");
            break;
        }
        for (int i = 0; i < ret; ++i)
        {
            int sockfd = events[i].data.fd;
            // 如果是监听端口
            if (sockfd == listenfd)
            {
                struct sockaddr_in client_address;
                socklen_t client_addrlen = sizeof(alient_address);
                int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlen);
                // 对每个非监听端口文件描述符都注册EPOLLONESHOT事件
                addfs(epollfd, connfd, true);
            }
            else if (events[i].events & EPOLLIN)
            {
                pthread_t thread;
                fds fds_for_new_worker;
                fds_for_new_worker.epollfd = epollfd;
                fds_for_new_worker.sockfd = sockfd;
                pthread_create(&thread, NULL worker, (void*), &fds_for_new_worker);
            }
            else
            {
                printf("something else happened\n");
            }
        }
    }

    close (listenfd);
    return 0;
}
```

## 非阻塞connect

由于程序用select等待连接完成，可以设置一个select等待时间限制，从而缩短connect超时时间。多数实现中，connect的超时时间在75秒到几分钟之间。有时程序希望在等待一定时间内结束，使用非阻塞connect可以防止阻塞75秒，在多线程网络编程中，尤其必要。 例如有一个通过建立线程与其他主机进行socket通信的应用程序，如果建立的线程使用阻塞connect与远程通信，当有几百个线程并发的时候，由于网络延迟而全部阻塞，阻塞的线程不会释放系统的资源，同一时刻阻塞线程超过一定数量时候，系统就不再允许建立新的线程（每个进程由于进程空间的原因能产生的线程有限），如果使用非阻塞的connect，连接失败使用select等待很短时间，如果还没有连接后，线程立刻结束释放资源，防止大量线程阻塞而使程序崩溃。

在对非阻塞socket调用connect，而连接没有立即建立时，在这种情况下，可以调用select、poll等函数来监听这个连接失败的socket上的可写事件。当select、poll等函数返回时，利用getsockopt来读取错误码并清除该socket上的错误码，如果错误码为0，则连接建立，否则连接失败，线程结束，连接失败。

```C++
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#define BUFFER_SIZE 1023

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_SETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

//超时连接函数，参数分别是服务器IP地址、端口号和超时时间。函数成功时返回已经处于连接状态的socket，失败则返回-1
int unblock_connect(const char* ip, int port, int time)
{
    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sockfd = socket(PF_INET, SOCK_STREAM, 0);
    int fdopt = setnonblocking(sockfd);l
    ret = connect(sockfd, (struct sockaddr*) &address, sizeof(address));
    if (ret == 0)
    {
        // 连接成功
        printf("connect with server immediatrly\n");
        fcntl(sockfd, F_SETFL, fdopt);
        return sockfd;
    }
    else if(errno != INPROGRESS)
    {
        // 如果连接没有立即建立，并且errno不是EINPROGRESS
        printf("unblock connect not support\n");
        return -1;
    }

    fd_set readfds;
    fd_set writefds;
    struct timeval timeout;

    FD_ZERO(&readfds);
    FDSET(sockfd, &writefds);

    timeout.tv_sec = time;
    timeout.tv_usec = 0;

    ret = select(sockfd + 1, NULL, &writefds, NULL, &timeout);
    if (ret <= 0)
    {
        // select超时或者出错，立即返回
        printf("connection time out\n");
        close(sockfd);
        return -1;
    }
    if (!FD_ISSET(sockfd, &writefds))
    {
        // socket不可写
        printf("no events on sockfd found\n");
        close(sockfd);
        return -1;
    }

    int error = 0;
    socklen_t length = sizeof(error);
    // 调用getsockopt来获取sockfd上的错误
    if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &length) < 0)
    {
        printf("get socket option failed\n");
        close (sockfd);
        return -1;
    }
    // 错误号不为0表示连接出错
    if (error != 0)
    {
        printf("connection failed after select with the error: %d \n", error);
        close(sockfd);
        return -1;
    }
    // 连接成功
    printf("connection ready after select with the socket: %d\n", sockfd);
    fcntl(sockfd, F_SETFL, fdopt);
    return sockfd;
}

int main(int argc, char *argv)
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(agrv[0]));
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi(argv[2]);
    
    int sockfd = unblock_connect(ip, port, 10);
    if (sockfd < 0)
    {
        return 1;
    }
    close(sockfd);
    return 0;
}
```