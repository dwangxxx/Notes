## http代理服务器的工作原理

在HTTP通信链上，客户端和目标服务器之间通常存在某些中转代理服务器，他们提供对目标资源的中转访问。一个HTTO请求可能被多个代理服务器转发，后面的服务器称为前面服务器的上游服务器。代理服务器按照其使用方式和作用，分为**正向代理服务器、反向代理服务器和透明代理服务器**。

**正向代理服务器**要求客户端自己设置代理服务器的地址，客户的每次请求都将直接发送到该代理服务器，并由代理服务器来请求目标资源。比如处于防火墙内的局域网机器要访问Internet，或者要访问一些被屏蔽掉的国外网站，就需要使用正向代理服务器。

**反向代理服务器**则被设置在服务器端，因为客户端无需进行任何设置。反向代理是指用代理服务器来接受Internet上的连接请求，然后将请求转发给内部网络上的服务器，并将从内部服务器上得到的结果返回给客户端。这种情况下，代理服务器对外就表现为一个真实的服务器。各大网站通常分区域设置了多个代理服务器，所以在不同的地方ping同一个域名可能得到不同的IP地址，因为这些IP地址实际上是代理服务器的IP地址。

**透明代理**只能设置在网关上。用户访问Internet的数据包必然都经过网关，如果在网关上设置代理，则该代理对用户来说是透明的。

代理服务器通常还提供缓存目标资源的功能，这样用户下次访问统一资源时速度很快。优秀的开源软件squid、varnish都是提供了缓存功能的代理服务器软件。

## cookie

HTTP是一种无状态的协议，即每个HTTP请求之间没有任何上下文关系。如果服务器处理后续HTTP请求时需要用到前面的HTTP请求的相关信息，客户端必须重传这些信息。这样就导致了HTTP请求必须传输更多的数据。

在交互式web应用程序兴起后，HTTP协议的这种无状态特性就显得不适应了，因为交互程序通常要承上启下。因此，我们要使用额外的手段来保持HTTP连接状态，常见的解决方法就是Cookie。Cookie是服务器发送给客户端的特殊信息(通过HTTP应答的头部字段设置"Set-Cookie")，客户端每次向服务器发送请求的时候都需要带上这些信息(通过HTTP请求的头部字段"Cookie")。这样服务器就可以区分不同的客户了。基于浏览器的自动登录就是Cookie实现的。

# 第五章：Linux网络编程基础API

## socket地址API

- 主机字节序与网络字节序

字节序分为大端字节序和小端字节序：大端字节序是指一个整数的高位字节存储在内存的低地址处，低位字节存储在内存的高地址处。小端字节序则相反。如下代码可以检查机器的字节序：
```C++
#include<stdio.h>

void byteorder()
{
    union {
        short value;
        char union_bytes[sizeof(short)];
    } test;

    test.value = 0x0102;
    if ((test.union_bytes[0] == 1) && (test.union_bytes[1] == 2))
    {
        printf("big endian\n");
    }
    else if ((test.union_bytes[0] == 2) && (test.union_bytes[1] == 1))
    {
        printf("little endian\n");
    }
    else
    {
        print("unknown...\n");
    }
}
```
现代PC大多采用小端字节序，因此小端字节序又被称为主机字节序。当格式化的数据在两台使用不同字节序的主机之间传递时，接收端必然错误的解释。解决办法是：发送端总是把要发送的数据转化为大端字节序数据之后再发送。大端字节序又称为网络字节序。Linux提供了如下四个函数来完成主机字节序和网络字节序之间的转换：
```C++
#include<netinet/in.h>

unsigned long int htonl(unsigned long int hostlong);
unsigned short int htons(unsigned short int hostshort);
unsigned long int ntohl(unsigned long int netlong);
unsigned short int ntols(unsigned short int netshort);
```

- 通用socket地址

socket网络编程接口中表示socket地址的是结构体sockaddr，其定义如下：
```C++
#include<bits/socket.h>

struct sockaddr {
    sa_family_t sa_family;
    char sa_data[14];
}
```
sa_family成员是地址族类型(sa_family_t)的变量。地址族类型通常与协议族类型对应。常见的协议族和对应的地址族如下表所示：
| 协议族 | 地址族 | 描述 |
| --- | --- | --- |
| PF_UNIX | AF_UNIX | UNIX本地域协议族 |
| PF_INET | AF_INET | TCP/IPv4协议族 |
| PF_INET6 | AF_INET6 | TCP/IPv6协议族 |

宏PF_\*和AF_\*都定义在bits/socket.h中，且二者具有完全相同的值，因此二者通常混用。sa_data成员用于存放socket地址值。

- 专用socket地址

Linux为每个协议族提供了专门的socket地址结构体:

UNIX本地与协议族：
```C++
#include<sys/un.h>
struct sockaddr_un
{
    sa_family_t sin_family;
    char sun_path[108];
};
```

TCP/IP协议族：
```C++
struct sockaddr_in
{
    sa_family_t sim_family; // 地址族
    u_int16_t sin_port;     // 端口号
    struct in_addr sin_addr;//IPv4地址结构体
};

struct in_addr
{
    u_int32_t s_addr;   // IPv4地址
};

struct sockaddr_in6
{
    sa_family_t sin6_family;
    u_int16_t sin6_port;
    u_int32_t sin6_flowinfo;
    struct in6_addr sin6_addr;
    u_int32_t sin6_scope_id;
};
struct in6_addr
{
    unsigned char sa_addr[16];
}
```

- IP地址转换函数

将点分IP地址转换成整数IP地址：
```C++
#include<arpa/inet.h>

// 将点分十进制字符串转换为用网络字节序整数表示的IPv4地址
in_addr_t inet_addr(const char *strptr);

// 与inet_addr函数一样，只是将返回装入inp指针指向的in_addr中
int inet_aton(const char *cp, struct in_addr *inp);

// 将网络地址转换为点分
char *inet_ntoa(struct in_addr in);

// af参数为协议族
int inet_pton(int af, const char *src, void *dst);

// cnt为指定目标存储单元的大小
const char *inet_ntop(int af, const void *src, char *dst, socklen_t cnt);

#define INET_ADDRSTRLEN 16
#define INET6_ADDRSTRLEN 46
```

## 创建socket

socket是一个可读、可写、可控制、可关闭的文件描述符。创建一个socket:
```C++
#include<sys/types.h>
#include<sys/socket.h>

int socket(int domain, int type, int protocol);

// domain: 协议族，PF_INET or PF_INET6 or PF_UNIX
// type: 服务类型，SOCKET_STREAM(TCP) or SOCKET_DGRAM(UDP) or SOCK_NONBLOCK

```

创建socket只是创建了一个socket和与之关联的sock结构(底层TCP是通过sock进行通信的)，并且创建了一个inode文件节点，但是并没有将其挂载到TCP的处理哈希表中，此时并不能进行通信，需要分配一个具体的地址，将其挂载到TCP的全局哈希表中才能让TCP进行通信。

## 命名socket

创建socket时，指定了地址族，但是并未指定使用该地址族中的哪个具体socket地址。将一个socket与socket地址绑定称为给socket命名。在服务器端，通常要命名socket。而在客户端，通常不需要命名socket，由操作系统自动分配socket地址。使用bind系统调用命名socket：
```C++
#include <sys/types.h>
#include <sys/socket.h>

int bind(int sockfd, const struct sockaddr *my_addr, socklen_t addrlen);
```

bind函数其实就是对上一步创建的socket进行一些参数的设置，以及将其sock结构挂载到TCP全局哈希表中。

## 监听socket

socket被命名之后，还不能马上接受客户端连接，需要使用如下系统调用来创建一个监听队列以存放待处理的客户连接：
```C++
#include<sys/socket.h>
int listen(int sockfd, int backlog);
```

sockfd指定被监听的socket，backlog参数提示内核监听队列的最大长度(属于监听套接字的)，如果监听队列的长度超过backlog，服务器将不再受理新的客户连接。

## 接受连接

从listen监听队列中接受一个连接：
```C++
#include<sys/types.h>
#include<sys/socket.h>

// sockfd为监听套接字的文件描述符
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
```

sockfd参数是执行过listen系统调用的监听socket。addr参数是用来获取被接受连接的远端socket地址，该socket地址的长度由addrlen指出。该系统调用会创建一个新的socket用于两端通信，返回socket的文件描述符。在调用accept时，底层TCP协议栈已经创建了一个sock结构，accept只需要创建一个新的socket结构并与之关联，并将其放入到文件表中即可。

accept返回一个新的连接socket，该socket唯一地标识了被接受的这个连接，服务器可通过读写该socket来与被接受连接对应的客户端通信。accept失败时返回-1并设置errno。

如果监听队列中处于established状态的连接对应的客户端出现网络异常，那么服务器对这个连接所执行的accept调用能成功返回，因为在调用accept时，底层TCP已经将sock结构体挂载到监听socket的队列中，accept函数需要做的就是创建一个新的socket套接字，然后将其与sock关联，此时客户端断开并不会使得队列中的sock消失，因此accept会成功返回。
```C++
#include<sys/socket.h>  // socket操作
#include<netinet/in.h>  // 字节序转换
#include<arpa/inet.h>   // 点分转整数
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>

int main(int argc, char *argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char *ip = argv[1];
    // 字符串转整数
    int port = atoi(argv[2]);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    // 将点分IP转换为整数地址，并将其赋给address.sin_addr
    inet_pton(AF_INET, ip, &address.sin_addr);
    // 网络字节序转换
    address.sin_port = htons(port);

    // 创建socket
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    // 将socket进行绑定
    int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
    assert(re1 != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    // 暂停20s以等待客户端连接和相关操作完成
    sleep(20);
    // 定义客户端连接socket
    struct sockaddr_in client;
    socklen_t client_addrlength = sizeof(client);
    // 执行accept操作，从listen队列中取出一个socket
    int connfd = accept(sock, (struct sockaddr*)&client, &client_addrlength);

    if (connfd < 0)
    {
        printf("errno is: %d\n", errno);
    }
    else
    {
        // 接受连接成功则打印出客户端的IP地址和端口号
        char remote[INET_ADDRSTRLEN];
        // 网络字节序到主机字节序
        // 点分转整数
        // int inet_pton(int af, char *src, void *dst);
        // 整数转点分
        // const char *inet_ntop(int af, void *src, char *dst, int sockaddr_len);
        printf("connected with ip: %s and port: %d\n", \
                inet_ntop(AF_INET, &client.sin_addr, remote, INET_ADDRSTRLEN), \
                ntohs(client.sin_port));
        close(connfd);
    }
    close (sock);
    return 0;
}
```
accept只是从监听队列中取出连接，而不论连接处于何种状态。

## 发起连接

服务器通过listen调用来被动接受连接，客户端需要通过如下调用来主动与服务器建立连接：
```C++
#include<sys/types.h>
#include<sys/socket.h>

int connect(int sockfd, const struct sockaddr *serv_addr, socklen_t addrlen);
```

## 关闭连接

```C++
#include<unistd.h>
int close(int fd);
```

close系统调用并非总是立即关闭一个连接，而是将fd的引用计数减1。只有当fd的引用计数为0时，才真正关闭连接。多进程程序中，一次fork系统调用默认将使父进程中打开的socket的引用计数加1,，因此必须在父子进程中都对该socket执行close调用才能将连接关闭。

可以使用shutdown调用立即终止连接：
```C++
#include<sys/socket.h>
int shutdown(int sockfd, int howto);
```

## 数据读写

- TCP数据读写

对文件的读写操作read和write同样适用于socket。但是socket编程接口提供了几个专门用于socket数据读写的系统调用，增加了对数据读写的控制。其中用于TCP流数据读写的系统调用是：
```C++
#include<sys/types.h>
#include<sys/socket.h>

ssize_t recv(int sockfd, void *buf, size_t len, int flags);
ssize_t send(int sockfd, const void *buf, size_t len, int flags);
```

其中flags的参数为数据收发提供了额外的控制。
| 选项名 | 含义 | send | recv |
| --- | --- | --- | --- |
| MSG_CONFIRM | 指示数据链路层协议持续监听对方的回应，直到得到答复。它仅能用于SOCK_DGRAM和SOCK_RAW的socket | Y | N |
| MSG_DONTROUTE | 不查看路由表，直接将数据发送给本地局域网内的主机。这表示发送者确切知道目标主机就在本地网络上 | Y | N |
| MSG_DONTWAIT | 对socket的此次操作将是非阻塞的 | Y | Y |
| MSG_MORE | 告诉内核应用程序还有更多数据要发送，内核将超时等待新数据写入TCP发送缓冲区后一并发送。这样可防止TCP发送过多小的报文段，从而提高传输效率 | Y | N |
| MSG_WAITALL | 读操作仅在读取到指定数量的字节后才返回 | N | Y |
| MSG_PEEK | 窥探读缓存中的数据，此次读操作不会导致这些数据被清除 | N | Y |
| MSG_OOB | 发送或者接受紧急数据 | Y | Y |

MSG_OOB选项可以给应用程序提供发送和接收带外数据的方法。TCP中没有真正的带外数据，只是提供了要讨论的紧急模式，TCP将数据放置在套接字发送缓冲区的下一个可用位置，并设置这个连接的紧急指针为下一个可用位置。接收端的带外缓冲只有1byte，因此发送端发送的多个字节带外数据只有最后1byte被当做带外数据。

- UDP数据读写

socket编程接口中用于UDO数据报读写的系统调用是：
```C++
#include<sys/types.h>
#include<sys/socket.h>

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags, struct sockaddr *src_addr, socklen_t *addrlen);
ssize_t sendto(int sockfd, const void *buf, size_t len, int flags, const struct sockaddr *dest_addr, socklen_t addr_len);
```

因为UDP通信没有连接的概念，所以每次读取数据的时候都需要获取发送端的socket地址。

## 带外标记

Linux内核检测到TCP紧急标记时，将通知应用程序有带外数据需要接收。内核通知应用程序带外数据到达的两种常见方式是：IO复用产生的异常事件和SIGURG信号。
```C++
#include<sys/socket.h>

int sockatmark(int sockfd);
```

此函数判断sockfd是否处于带外标记，即下一个被读取到的数据是否是带外数据。

## 地址信息函数

获取一个连接socket的本地socket地址以及远端的socket地址，如下函数可以实现：
```C++
#include<sys/socket.h>

int getsockname(int sockfd, struct sockaddr *address, socklen_t *address_len);
int getpeername(int sockfdm struct sockaddr *address, socklen_t *address_len);
```

## socket选项

fcntl系统调用是控制文件描述符属性的通用POSIX方法，用法：
```C++
fcntl(fd, F_SETOWN, pid);
// 将fd的持有者设置为进程pid
```

下面两个系统调用专门用来读取和设置socket文件描述符属性：
```C++
#include<sys/socket.h>

int getsockopt(int sockfd, int level, int option_name, void *option_value, socklen_t *restrict option_len);
int setsockopt(int sockfd, int level, int option_name, const void *option_value, socklen_t option_len);
```

sockfd参数指定被操作的目标socket。level参数指定要操作哪个协议的选项，比如IPv4、IPv6、TCP等。option_name参数则指定选项的名字。option_value和option_len参数分别是被操作选项的值和长度。

对于服务器而言，有部分socket选项只能在调用listen调用前针对监听socket设置才有效。这是因为连接socket只能由accept调用返回，而accept从listen监听队列中接受的连接至少已经完成了TCP三次握手的前两个步骤，这说明服务器已经往被接受连接上发出了TCP同步报文段。但是有的socket选项是应该在TCP同步报文段中设置。因此，需要对监听socket设置这些socket选项，那么accept返回的连接socket将自动继承这些选项。

- SO_REUSEADDR选项

TCP连接的TIME_WAIT状态，如果一个TCP连接处于TIME_WAIT状态，那么不能继续连接，因为一个端口只能创建一个连接(确定的source ip, source port, dest ip, dest port)。如果服务器端处于TIME_WAIT状态时突然宕机，那么重启时不能立即重启服务，因为此时端口已经被TIME_WAIT状态的socket给占用了。服务器可以通过设置socket选项SO_REUSEADDR来强制使用被处于TIME_WAIT状态的连接占用的socket地址。
```C++
int sock = socket(PF_INET, SOCK_STREAM, 0);
assert(sock >= 0);
int reuse = 1;
setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

struct sockaddr_int address;
bzero(&address, sizeof(address));
address.sin_family = AF_INET;
inet_pton(AF_INET, ip, &address.sin_addr);
address.sin_port = htons(port);
int ret = bind(sock, (struct sockaddr *)&address, sizeof(address));
```

经过setsockopt的设置之后，即使sock处于TIME_WAIT状态，与之绑定的socket地址也可以立即被重用。但是监听的端口不能同时被使用。

- SO_RCVBUF和SO_SNDBUF选项

SO_RCVBUF和SO_SNDBUF选项分别表示TCP接收缓冲和发送缓冲区的大小。当我们使用setsockopt来设置TCP的接收缓冲区的大小时，系统都会将其值加倍，并且不得小于某个值。TCP接收缓冲区的最小值是256字节，而发送缓冲区的最小值是2048字节。

- SO_RCVLOWAT和SO_SNDLOWAT选项

SO_RCVLOWAT和SO_SNDLOWAT选项分别表示TCP接收和发送缓冲区的低水位标志。他们一般被IO复用系统调用来判断socket是否可读或可写。当TCP接收缓冲区中可读数据的总数大于其低水位标志时，IO复用系统调用将通知应用程序可以从对应的socket上读取数据；当TCP发送缓冲区中的空闲空间大于其低水位标志时，IO复用系统调用将通知应用程序可以往对应的socket上写入数据。默认情况下都为1。

- SO_LINGER选项

SO_LINGER选项用于控制close系统调用在关闭TCP连接时的行为。默认情况下，当我们使用close系统调用来关闭一个socket时，close将立即返回，TCP模块负责把该socket对应的TCP发送缓冲区中残留的数据发送给对方。设置SO_LINGER选项时，需要给setsockopt传递一个linger类型的结构体：
```C++
#include<sys/socket.h>
struct linger
{
    int l_onoff; //开启或关闭
    int l_linger;   // 滞留时间
}
```
产生以下三种行为：
> l_onoff为0，关闭，使用默认行为；
> l_onoff不为0，l_linger为0。此时close系统调用立即返回，TCP模块将丢弃被关闭的socket对应的TCP发送缓冲区中残留的数据，同时给对方发送一个复位报文段。因此，这种情况给服务器提供了异常终止一个连接的方法。
> l_onoff不为0，l_linger不为0。此时close的行为取决于两个条件：一时被关闭的socket对应的TCP发送缓冲区是否还有残留数据；二是socket是阻塞还是非阻塞的。对于阻塞的socket，close调用将等待一段长为l_linger的时间，直到TCP模块发送完所有残留数据并得到对方确认。如果socket是非阻塞的，close立即返回，此时需要根据其返回值和errno来判断残留数据是否发送完毕。

## 网络信息API

- gethostbyname和gethostbyaddr

gethostbyname函数根据主机名获取主机的完整信息，gethostbyaddr函数根据IP地址获取主机的完整信息：
```C++
#include<netdb.h>

struct hostent* gethostbyname (const char *name);
struct hostent* gethostbyaddr (const void *addr, size_t len, int type);
```
函数返回hostent结构体类型的指针：
```C++
#include<netdb.h>

struct hostent
{
    char *h_name;   // 主机名
    char **h_aliases;   // 主机别名列表
    int h_addrtype; //地址类型
    int h_length;   // 地址长度
    char **h_addr_list; //按照网络字节序列出的主机IP地址列表
};
```

- getservbyname和getservbyport

getservbyname函数根据名称获取某个服务的完整信息，getservbyport函数根据端口号获取某个服务的完整信息。实际上都是通过读取/etc/services来获取服务的信息的。返回servnet结构体指针：
```C++
#include<netdb.h>
struct servent
{
    char *s_name;   //服务名称
    char **s_aliases;   // 服务别名列表
    int s_port; //端口号
    char *s_proto;  // 服务类型
};
```

- getaddrinfo

getaddrinfo既能通过主机名获取IP地址，也能通过服务名获取端口号：
```C++
#include<netdb.h>

int getaddrinfo(const char *hostname, const char *service, const struct addrinfo *hints, struct addrinfo **result);
```

hostname参数可以接收主机名，也可以接收字符串表示的IP地址。service参数可以接收服务名，也可以接收字符串表示的十进制端口号。

# 高级IO函数

## pipe函数

pipe函数可用于创建一个管道，以实现进程间通信：
```C++
#include<unistd.h>
int pipe(int fd[2]);
```

通过pipe函数创建的这两个文件描述符fd[0]和fd[1]分别构成管道的两端，往fd[1]写入的数据可以从fd[0]读出。管道是单通道的，不能双向通信。默认情况下，这一对文件描述符都是阻塞的。如果管道的写端文件描述符fd[1]引用计数减少至0，即没有任何进程需要往管道中写入数据，则针对该管道的读端文件描述符fd[0]的read操作将返回0；反之，如果管道的读端文件描述符fd[1]引用计数减少为0，则针对改管道的写端文件fd[1]的write操作将失败。

在socket的基础API中有一个socketpair函数，可以创建双向管道：
```C++
#include<sys/types.h>
#include<sys/socket.h>

int socketpair(int domain, int type, int protocol, int fd[2]);
```

## dup函数和dup2函数

有时需要将标准输入重定向到一个文件，或者把标准输出重定向到一个网络连接。可以通过下面用于复制文件描述符的dup或dup2实现：
```C++
#include<unistd.h>

int dup(int file_descriptor);
int dup2(int file_descriptor_one, int file_descriptor_two);
```
dup函数创建一个新的文件描述符，该新文件描述符和原有文件描述符file_descriptor指向相同的文件、管道或者网络连接。并且dup返回的文件描述符总是取系统当前可用的最小整数值。

- readv函数和writev函数

readv函数将数据从文件描述符读到分散的内存块中，即分散读；writev函数则将多块分散的内存数据一并写入文件描述符中，即集中写：
```C++
#include<sys/uio.h>

ssize_t readv(int fd, const struct iovec *vector, int count);
ssize_t writev(int fd, const struct iovec *vector, int count);

struct iovec 
{
    ptr_t iov_base; // 起始地址
    size_t iov_len; // 长度
};
```

## sendfile函数

sendfile函数在两个文件描述符之间直接传递数据(完全在内核中操作)，从而避免了内核缓冲区和用户缓冲区之间的数据拷贝，效率很高，被称为零拷贝：
```C++
#include<sys/sendfile.h>

ssize_t sendfile(int out_fd, int in_fd, off_t *offset, size_t count);
```

in_fd表示待读入内容的文件描述符，out_fd表示待写入内容的文件描述符。in_fd必须是一个支持类似mmap函数的文件描述符，即它必须指向真实的文件，不能是socket或者管道；而out_fd则必须是一个socket。因此sendfile几乎是专门为在网络上传输文件而设计的。例子：
```C++
#include<sys/socket.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<assert.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<errno.h>
#include<string.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<sys/sendfile.h>

int main(int argc, char *argv[])
{
    const char *ip = argv[1];
    int port = atoi(argv[2]);
    const char* file_name = argv[3];

    int filefd = open(file_name, O_RDONLY);

    assert(filefd > 0);
    // 文件状态结构体
    struct stat stat_buf;
    // 获取文件状态
    fstat(filefd, &stat_buf);

    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    assert(sock >= 0);

    int ret = bind(sock, (struct sockaddr*) &address, sizeof(address));
    assert(ret != -1);

    ret = listen(sock, 5);
    assert(ret != -1);

    struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
    int connfd = accept(sock, (struct sockaddr*)&client, &client_len);
    if (connfd < 0)
    {
        printf("errno is: %d\n", errno);
    }
    else
    {
        sendfile(connfd, filefd, NULL, stat_buf.st_size);
        close(connfd);
    }

    close(sock);
    return 0;
}
```


## mmap和munmap函数

mmap函数用于申请一段内存空间。我们可以将这段内存作为进程间通信的共享内存，也可以将文件直接映射到其中。munmap函数释放由mmap创建的这段内存空间：
```C++
#include<sys/mman.h>
void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
int munmap(void *start, size_t length);
```
- start: 设置某个特定的地址作为这段内存的起始地址，如果为空则自动分配一个地址。
- length：指定内存段的长度。
- prot：设置内存段的访问权限(PROT_READ, PROT_WRITE, PROT_EXEC, PROT_NONE)。
- flags：控制内存段内容被修改后程序的行为。(MAP_SHARED, MAP_PRIVATE, MAP_ANONYMOUS, MAP_FIXED, MAP_HUGETLB)。
- fd：文件描述符。
- offset：设置从文件的何处开始映射。

mmap成功时返回指向目标内存区域的指针，失败时返回MAP_FAILED((void *) -1)并设置errno。

## splice函数

splice函数用于在两个文件描述符之间移动数据，也是零拷贝操作。splice函数的定义如下：
```C++
#include<fcntl.h>
ssize_t splice(int fd_in, loff_t *off_in, int fd_out, loff_t *off_out, size_t len, unsigned int flags);
```

flags控制数据如何移动: SPLICE_F_MOVE, SPLICE_F_NONBLOCK, SPLICE_F_MORE, SPLICE_F_GIFT。

**使用splice函数时，fd_in和fd_out必须至少有一个是管道文件描述符**。splice函数调用成功时返回移动字节的数量。下面使用splie函数实现一个零拷贝的回射服务器，将客户端发来的数据原样返回：
```C++
int fd[2];
assert(ret != -1);
ret = pipe(fd);
// 将connfd上流入的客户数据定向到管道中
ret = splice(connfd, NULL, fd[1], NULL, 32768, SPLICE_MORE | SPLICE_F_MOVE);
assert(ret != -1);

// 将管道的输出定向到connfd客户连接文件描述符
ret = splice(fd[0], NULL, connfd, NULL, 32768, SPLICE_F_MORE | SPLICE_F_MOVE);
assert(ret != -1);
close(connfd);
```

## tee函数

tee函数在两个管道文件描述符之间复制数据，也是零拷贝操作。**它不消耗数据，因此源文件描述符上的数据仍然可以用于后续读操作**：
```C++
#include<fcntl.h>
ssize_t tee(int fd_in, int fd_out, size_t len, unsigned int flags);
```

tee函数中，**fd_in和fd_out都必须是管道文件描述符**。可以使用slice函数和tee函数实现同时将数据输出到标准输出以及文件中。

## fcntl函数

fcntl函数提供了对文件描述符的各种控制操作(另外一个常见的控制文件描述符属性和行为的系统调用是ioctl)。fcntl定义如下：
```C++
#include<fcntl.h>

int fcntl(int fd, int cmd, ...);
```

cmd参数指定执行何种类型的操作，根据操作类型的不同，该函数可能还需要第三个可选参数arg。cmd参数列表:

**F_DUPFD, F_DUPFD_CLOSEXEC, F_GETFD(获取fd标志), F_SETFD, F_GETFL(获取fd的状态标志), F_SETFL, F_GETOWN, F_SETOWN(设定SIGIO和SIGURG信号的宿主进程PID), F_GETSIG, F_SETSIG, F_SETPIPE(设置管道容量), F_GETPIPE**。

在网络编程中，fcntl函数通常用来将一个文件描述符设置为非阻塞的：
```C++
int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}
```

**注：SIGIO和SIGURG这两个信号与其他Linux信号不同，它们必须与某个文件描述符相关联才能使用：当被关联的文件描述符可读或者可写时，系统将触发SIGIO信号；当被关联的文件描述符(socket)上有带外数据时，系统将触发SIGURG信号**。将信号和文件描述符关联的方法，就是使用fcntl函数为目标文件描述符指定宿主进程或进程组，那么被指定的宿主进程或进程组将捕获这两个信号。

# Linux服务器程序规范

除了网络通信之外，服务器程序通常还必须考虑许多其他的问题：
- Linux服务器程序一般以后台进程形式运行。又称为守护进程(daemon)。
- Linux服务器程序通常有一套日志系统，它至少能输出日志到文件，有的高级服务器还能输出日志到专门的UDP服务器。大部分后台进程都在/var/log目录下拥有自己的日志目录。
- Linux服务器程序一般以某个专门的非root身份运行。比如mysql、httpd、syslogd等，分别拥有自己的运行账户。
- Linux服务器程序通常是可配置的。服务器程序通常能处理很多命令行选项，如果一次运行的选项太多，就可以使用配置文件来管理。大多数服务器都有自己的配置文件，并存放在/etc/目录下。
- Linux服务器进程通常会在启动的时候生成一个PID文件并存入/var/run目录中，以记录该后天进程的PID。
- Linux服务器程序通常需要考虑系统资源和限制，以预测自身能承受的最大负荷，比如进程可用文件描述符总数和内存总量等。

## Linux系统日志

用户进程通过调用*syslog*函数生成系统日志，该函数将日志输出到一个UNIX本地域socket类型的文件 **/dev/log** 中，rsyslogd监听该文件以获取用户进程的输出。rsyslogd利用额外的模块实现了内核日志的功能。内核日志由printk等函数打印至内核的环状缓冲区中，环状缓冲区的内容直接映射到 **/proc/kmsg** 文件中，rsyslogd则通过读取该文件获得内核日志。

rsyslogd守护进程在接收到用户进程或者内核输入的日志后，会把他们输出至某些特定的日志文件。默认下：调试信息(/var/log/debug, 普通信息/var/log/message, 内核信息/var/log/kern.log)。

- syslog函数

```C++
#include<syslog.h>
void syslog(int priority, const char *message, ...);

// priority是设施值与日志级别的按位与。设施值默认为LOG_USER
// 日志级别
#include<syslog.h>
#define LOG_EMERG   0   // 系统不可用
#define LOG_ALERT   1   // 报警
#define LOG_CRIT    2   // 非常严重错误
#define LOG_ERR     3   // 错误
#define LOG_WARNING 4   // 警告
#define LOG_NOTICE  5   // 通知
#define LOG_INFO    6   // 信息
#define LOG_DEBUG   7   // 调试

// 使用如下函数改变syslog的默认输出方式
void openlog(const char *ident, int logopt, int facility);

// logopt
#define LOG_PIG     0x01    // 包含PID
#define LOG_CONS    0x02    // 打印至终端
#define LOG_ODELAY  0x04    // 延迟
#define LOG_NDELAY  0x08    // 不打开延迟

// 使用如下函数过滤日志
int setlogmask(int maskpri);
// 使得日志级别大于日志掩码的日志信息被系统忽略
```

## 用户信息

- UID、EUID、GOD和EGID

```C++
#include<sys/types.h>
#include<unistd.h>
uid_t getuid();     // 获取真实用户ID
uid_t geteuid();    // 获取有效用户ID
gid_t getgid();     // 获取真实组
gid_t getegid();    // 获取有效组
int setuid(uid_t uid);  // 设置真实用户ID
int setegid(uid_t uid); // 设置有效用户ID
int setgid(gid_t gid);  // 设置真实组ID
int setegid(gid_t gid); // 设置有效组ID
```
一个进程拥有两个用户ID: UID和EUID。EUID存在的目的是方便资源访问：它使得运行程序的用户拥有该程序的有效用户的权限。比如普通用户运行su程序之后，其EUID编程root，就可以执行root操作。EGID含义与EUID类似：给运行目标程序的组用户提供有效组的权限。

## 进程间关系

- 进程组

进程组ID:
```C++
#include<unistd.h>
pid_t getpgid(pid_t pid);
```
每个进程组都有一个首领进程，其PGID和PID相同。进程组将一直存在，知道其中所有进程都退出，或者加入到其他进程组:
```C++
#include<unistd.h>
int setpgid(pid_t pid, pid_t pgid);
```

一个进程只能设置自己或者其子进程的PGID，并且当子进程调用exec系列函数后，我们也不能再在父进程中对它设置PGID。

## 系统资源限制

Linux系统资源限制可以通过以下一对函数来读取和设置：
```C++
#include<sys/resource.h>

int getrlimit(int resource, struct rlimit *rlim);
int setrlimit(int resource, const struct rlimit *rlim);

struct rlimit
{
    rlim_t rlim_cur;
    rlim_t rlim_max;
};
```

rlim_cur指定成员的软限制，rlim_max指定成员的硬限制。软限制是一个建议性的、最好不要超越的限制，如果超越的话，系统可能向进程发送信号以终止其运行。硬限制一般是软限制的上限。

resource指定资源类型：
> RLIMIT_AS: 进程虚拟内存总量限制（单位字节）
> RLIMIT_CORE: 进程核心转储文件的大小限制（单位字节）
> RLIMIT_DATA: 进程数据段限制（单位字节）
> RLIMIT_FSIZE: 文件大小限制（单位字节），超过该限制将使得某些函数(如write)产生EFBIG错误
> RLIMIT_NOFILE: 文件描述符数量限制
> RLIMIT_NPROC: 用户能创建的进程数限制
> RLIMIT_SIGPENDING: 用户能挂起的信号数量限制
> RLIMIT_STACK: 进程栈内存限制（单位字节）

## 改变工作目录和根目录

获取进程当前工作目录和改变进程工作目录的函数分别是：
```C++
#include<unistd.h>
char *getcwd(cahr *buf, size_t size);
int chdir(const char *path);
```

改变进程根目录：
```C++
int chroot(const char *path);
```

# 第八章：高性能服务区程序框架

## 两种高效的事件处理模式

- reactor模式

reactor模式要求主线程只负责监听文件描述符上是否有事件发生，有的话就立即将该工作事件通知工作线程。初次之外，主线程不做任何其他实质性的工作。读写数据，接受新的连接，以及处理客户请求均在工作线程中完成。

使用同步IO模型(以epoll_wait为例)实现的reactor模式的工作流程是：
> 主线程往epoll内核事件中注册socket上的读就绪事件（主线程负责循环等待监听/连接socket上的事件，监听socket的事件由主线程处理，连接socket上的读写事件交由工作线程处理）；
> 主线程调用epoll_wait等待socket上有数据可读；
> 当socket上有数据可读时，epoll_wait通知主线程。主线程则将socket可读事件放入请求队列；
> 睡眠在请求队列上的某个工作线程被唤醒，它从socket读取数据，并处理客户请求，然后往epoll内核事件表中注册该socket上的写就绪事件；
> 主线程调用epoll_wait等待socket可写；
> 当socket可写时，epoll_wait通知主线程。主线程将socket可写事件放入请求队列；
> 睡眠在请求队列上的某个工作线程被唤醒，它往socket上写入服务器处理客户请求的结果。

- proactor模式

与reactor模式不同，proactor模式将所有的IO操作都交给主线程和内核来处理，工作线程仅仅负责业务逻辑。

使用异步IO模型(以aio_read和aio_write为例)实现的proactor模式的工作流程是：

> 主线程调用aio_read函数向内核注册socket上的读完成事件，并告诉内核用户读缓冲的位置，以及读操作完成时如何通知应用程序(以信号为例)。
> 主线程继续处理其他逻辑。
> 当socket上的数据被读入用户缓冲区后，内核向应用程序发送一个信号，以通知应用程序数据已经可用。
> 应用程序预先定义好的信号处理函数选择一个工作线程来处理客户请求。工作线程处理完用户请求之后，调用*aio_write*函数向内核注册socket上的写完成事件，并告诉内核用户写缓冲区的位置，以及写操作完成时如何通知应用程序。
> 主线程继续处理其他逻辑。
> 当用户缓冲区的数据被写入socket之后，内核向应用程序发送一个信号，以通知应用程序数据已经发送完毕。
> 应用程序预先定义好的信号处理函数选择一个工作线程来做善后处理，比如决定是否关闭socket。

# 第九章：信号

## SIGHUP

当挂起进程的控制终端时，SIGHUP信号将被触发。对于没有控制终端的网络后台程序而言，它们通常利用SIGHUP信号来强制服务器重读配置文件。例如xinetd程序。

xinetd程序在接收到SIGHUP信号之后将调用hard_config函数，它循环读取/etc/xinetd.d/目录下的每个子配置文件，并检测其变化。如果某个正在运行的子服务的配置文件被修改以停止服务，则xinted主进程将给孩子服务进程发送SIGTERM信号结束服务。

## SIGPIPE

默认情况下，往一个读端关闭的管道或者socket写数据将引发SIGPIPE信号。我们需要在代码中捕获并处理该信号，或者至少忽略它，因为程序接收到SIGPIPE信号的默认行为是结束进程，而我们绝不希望因为错误的写操作而导致程序退出。引起SIGPIPE信号的写操作将设置errno为EPIPE。

对于SIGPIPE信号，我们可以使用send函数的MSG_NOSIGNAL标志来禁止写操作触发SIGPIPE信号，在这种情况下，应该使用send函数反馈的errno值来判断管道或者socket连接的读端是否已经关闭。

此外，还可以使用IO复用系统调用来检测管道和socket连接的读端是否已经关闭。以poll为例，当管道的读端关闭时，写端文件描述符上的POLLHUP事件将被触发；当socket连接被对方关闭时，socket上的POLLRDHUP事件将被触发。

# 第十章：定时器

Linux提供了三种定时方法：
> socket选项SO_RCVTIMEO和SO_SNDTIMEO
> SIGALARM信号
> IO复用系统调用的超时参数

## socket选项SO_RCVTIMEO和SO_SNDTIMEO

socket选项SO_RCVTIMEO和SO_SNDTIMEO分别用来设置socket接收数据超时时间和发送数据超时时间，这两个选项仅对与数据接受和发送相关的socket专用系统调用。(包括send, sendmsg, recv, recvmsg, accept, connect)。

