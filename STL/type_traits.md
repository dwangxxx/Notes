## __type_traits

STL中，只对迭代器加以规范，制定了*iterator_traits(即迭代器萃取器)*这样的东西。SGI将其拓展到迭代器意外的东西，就是所谓的*__type_traits*。*iterator_traits*负责萃取迭代器的特性，*__type_traits*则负责萃取型别的特性。

在我们需要在内存上填上一些数据时，我们可以先判断这个型别是否具有*non-trivial default constructor*？是否具有*non-trivial copy constructor*？是否具有*non-trivial assignment operator*？是否具有*non-trivial deconstructor*？如果没有的话，我们在对这个型别进行构造、析构、拷贝、赋值等操作时，就可以采用最有效的方式，不用去调用一些无用的构造函数。这对于大规模并且操作频繁的容器，有着显著的效率提升。

定义在SGI STL<type_traits.h>中的__type_traits，提供了一种机制，允许针对不同的型别属性，在编译期间完成函数派送决定。这对于撰写template很有帮助，例如，当我们准备对一个元素型别未知的数组执行copy操作时，如果我们事先知道其元素型别是否有一个*trivial copy constructor*，就能帮助我们决定是否可用快速的*memcpy()或者memmove()**。

根据*iterator_traits*，我们希望程序中可以这样运用*__type_traits<T>*，其中T代表任意型别：
```C++
__type_traits<T>::has_trivial_default_constructor
__type_triats<T>::has_trivial_copy_constructor
__type_traits<T>::has_trivial_assignment_operator
__type_traits<T>::has_trivial_destructor
__type_traits<T>::is POD_type   // 原始数据
```

其返回结果应该是个对象，因为需要其结果来进行参数推导，编译器只有面对class object形式的参数才会做参数推导，因此定义：
```C++
struct __true_type{};   // 标示真
struct __false_type{};  // 标示假
```

因此，在**__type_traits**内定义一些typedef来完成类型的萃取:
```C++
template<class T>
struct __type_traits {
    typedef __true_type this_dummy_member_must_be_first;
    typedef __false_type has_trivial_default_constructor;
    typedef __false_type has_trivial_copy_constructor;
    typedef __false_type has_trivial_assignment_constructor;
    typedef __false_type has_trivial_destructor;
    typedef __false_type is_POD_type;
};
```

以上处于保守的做法，将所有成员都定义成*__false_type*，下面将对所有C++标量类型定义__type_traits的特化版本：
```C++
__STL_TEMPLATE_NULL struct __type_traits<char> {
    typedef __true_type has_trivial_default_constructor;
    typedef __true_type has_trivial_copy_constructor;
    typedef __true_type has_trivial_assignment_operator;
    typedef __true_type has_trivial_destructor;
    typedef __true_type is_POD_type;
};
// 其余类似

template<class T>
struct __type_traits<T*> {
    typedef __true_type has_trivial_default_constructor;
    typedef __true_type has_trivial_copy_constructor;
    typedef __true_type has_trivial_assignment_operator;
    typedef __true_type has_trivial_destructor;
    typedef __true_type is_POD_type;
}
```

__type_traits例子，例如STL中的**uninitalized_fill_n()**全局函数:
```C++
template<class ForwardIterator, class Size, class T>
inline ForwardIterator uninitialized_fill_n(ForwardIterator first, Size n, const T& x) {
    return __uninitialized_fill_n(first, n, x, value_type(first));
}
```
上述函数中的*value_type()*函数首先萃取出迭代器的value type，然后再利用*__type_traits*判断该型别是否为POD型别：
```C++
template <class ForwardIterator, class Size, class T, class T1>
inline ForwardIterator __uninitialized_fill_n(ForeardIteartor first, Size n, const T& xx, T1 *) {
    typedef typename __type_traits<T1>::is_POD_type is_POD;
    return __uninitialized_fill_n_aux(first, n, x, is_POD());
}
```

然后就根据是否为POS型别采取最适当的措施:

- 如果不是POD型别，就会派送到如下函数:
```C++
template<class ForwardIterator, class Size, class T>
ForwardIterator __uninitialized_fill_n_aux(ForwardIterator first, Size n, const T& x, __false_type) {
    Forward cur = first;
    for (; n > 0; --n, ++cur)
        construct(&*cur, c);

    return cur;
}
```
- 如果是POD型别，就会派送到如下函数：
```C++
template<class ForwardIterator, class Size, class T>
inline ForwardIterator __uninitialized_fill_n_aux(ForwardIterator first, Size n, const T& x, __true_type) {
    return fill_n(first, n, x);
}

template<class OutputIterator, class Size, class T>
OutputIterator fill_n(OutputIterator first, Size n, const T& value) {
    for (; n > 0; --n, ++first)
        *first = value;
    
    return first;
}
```