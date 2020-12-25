## linux文件IO

- open()调用

通过open()系统调用来打开一个文件并获得一个文件描述符。
```C++
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

int open(const char *name, int flags);
int open(const char *name, int flags, mode_t mode);
```

open系统调用将路径名name给出的文件与一个成功返回的文件描述符相关联，文件位置指针被设置为零，而文件则根据flags给出的标志位打开。

open的*flag*参数必须是以下之一：O_RDONLY，O_WRONLY或者O_RDWR。还可以与其他flag进行或操作

- creat()调用

```C++
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

int creat(const char *name, mode_t mode);

int fd;
fd = creat(file, 0644);
// 等价于：
int fd;
fd = open(file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
```

- read()调用

```C++
#include <unistd.h>

ssize_t read(int fd, void *buf, size_t len);
```

该系统调用从由fd指向的文件的当前偏移量至多读len个字节到buf中。成功时，将返回写入buf中的字节数。出错时将返回-1，并设置errno。

在进行非阻塞读的时候，需要检查errno，当设置read()为非阻塞读的时候，read()会返回-1，并且设置errno为EAGAIN。因此在非阻塞读的时候，需要检查errno是否为EAGAIN。

- write()调用

```C++
#include<unistd.h>

ssize_t write(int fd, const void *buf, size_t count);
```

一个write调用从由文件描述符fd引用文件的当前位置开始，将buf中至多count个字节写入到文件中。不支持定位的文件总是从开头写。成功时，返回写入字节数，并更新文件位置。错误时，返回-1，并将errno设置为相应的值。