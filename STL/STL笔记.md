# STL实现原理

## 迭代器

迭代器有五种类型：
- Input Iterator: 只能单步前向迭代元素，不允许修改该类迭代器引用的对象
- Output Oterator: 与Input Iterator相似，但是具有写的能力
- Forward Iterator: 该类迭代器可以在一个正确的区间进行读写操作，它拥有Input Iterator的所有特性，和OutputIterator的部分特性，以及单步前向迭代元素的能力
- Bidirectional Iterator: 该类迭代器是Forward Iterator的基础上提供了单步向后迭代元素的能力
- Random Access Iterator: 该类迭代器能完成上面所迭代器的工作，并且能像指针一样进行算数计算，而不是仅仅只有单步向前或向后迭代

**vector和deque**提供的是Random Access Iterator，**list**提供的是Bidirectional Iterator, **set和map**提供的是Forward Iterator.

## vctor

- 是内存可2倍增长的动态数组
- **数据结构：线性连续空间**
- 维护三个迭代器：start、finish、end_of_storage

增加元素时，如果空间不够，则扩充容量至原来的两倍。如果两倍容量仍不足，就扩充足够大的容量。并且容量的扩张必须经历：重新配置申请内存、元素移动、释放原空间等操作。对vector的任何操作，如果引起空间的重新分配，就会导致指向原vector的所有迭代器都失效了。

## deque

- **deque使用分段的内存存储数据，每一段内的的内存是连续的，段之间的内存不是连续的，是动态申请的。**
- deque使用一个map来进行存储每一段数据的起始地址。map就是一段连续的数据空间，里面存储了每段数据的内存地址。deque维护两个迭代器，start和finish迭代器，start迭代器里面有其实数据算的map索引，以及数据段中的起始地址first、终点地址last和当前可插入的地址值cur。
- 调用begin的时候，返回start迭代器，调用end的时候，返回finish迭代器。

## list

- **list使用的底层数据结构是环状双端链表**。
- 插入和结合操作都不会造成原来list的迭代器失效，删除操作仅仅使得“指向被删除元素”的迭代器失效，其他迭代器不受影响。
- 随机访问比较慢。

## set

- 底层数据结构使用平衡的红黑树
- 插入删除操作时仅仅需要指针操作节点即可完成，不涉及内存移动和拷贝
- set中的元素都是唯一的，而且默认情况下会对元素自动进行升序排序
- set内部元素也是以键值对的方式存储的，它的键值和实值相同
- set中不允许存放两个实值相同的元素
- 迭代器被定义成const iterator，说明set不允许通过迭代器修改set的值

## multiset

- 数据结构和set一样，采用了红黑树
- 允许插入重复的键值，使用insert_equal()机制
- 插入、删除操作的时间复杂度为O(log2n)

## map
- map中key的键是唯一的
- 数据结构为红黑树
- 提供基于key的快速检索能力
- 元素的插入是按照顺序规则插入的，不能指定位置插入
- 对于迭代器来说，可以修改实值，但是不能修改key
- 根据key快速查找，查找的复杂度是log2n

## multimap
- 与map相同，但是可以包含重复键

**map和multimap的返回值不同，multimap总能插入成功，因此返回迭代器，而map返回pair<iterator, bool>，第二个参数表示是否插入成功。**

## allocator

allocator除了负责内存的分配和释放，还负责对象的构造和析构，知道类型才能调用对象的构造和析构函数。对于内存分配，allocator接口设计中有类似于“为n个类型为T的对象分配内存”这种批量操作，这就需要知道类型才能算出对象需要的空间。

我们可以按照STL标准实现一个allocator，就可以搭配STL里面的容器使用了。

函数模板使用时不需要进行实例化，即传入参数就可以；但是类模板使用时需要制定类型，class_type<type A, type B>;

## STL的萃取机制traits

是迭代器和算法之间的一个接口。算法和容器之间是分别设计的，在算法内部可能会需要迭代器指向对象的一些型别，如果需要型别作为函数的返回类型，那么用普通的模板函数推导机制无法完成，因为函数模板无法完成函数返回类型的自动推导。因此设计了一个迭代器萃取机制，萃取出迭代器中的型别。迭代器中一共有五种型别。

STL的萃取机制主要是应用了C++的模板函数自动推导机制以及偏特化机制。可以为原生指针偏特化一个萃取类。

SGI_STL，SGI STL中使用了__type_traits来萃取型别的特性，iterator_traits来萃取迭代器的特性。