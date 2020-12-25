## 标准库函数 std::move
编译器只对右值引用才能调用转移构造函数和转移赋值函数，而所有命名对象都只能是左值引用，如果已知一个命名对象不再被使用而想对它调用转移构造函数 和转移赋值函数，也就是把一个左值引用当做右值引用来使用，可以使用std::move函数，这个函数以非常简单的方式将左值引用转换为右值引用。
示例程序：
```C++
void ProcessValue(int& i) {
    std::cout << "LValue processed: " << i << std::endl;
}

void ProcessValue(int&& i) {
    std::cout << "RValue processed: " << i << std::endl;
}

int main() {
    int a = 0;
    ProcessValue(a);
    ProcessValue(std::move(a));
}

运行结果：
LValue processed: 0 
RValue processed: 0
```
std::move在提高swap函数的性能上非常有帮助，一般来说，swap函数的通用定义如下：
```C++
template <class T> swap(T& a, T& b)
{
    T tmp(a);   //copy a to tmp
    a = b;      // copy b to a
    b = tmp;    // copy tmp to b
}
// 上面的函数需要调用三次拷贝构造操作，并且需要调用三次析构函数，性能损失比较严重
```
有了std::move，swap函数定义可变为：
```C++
template <class T> swap(T& a, T& b)
{
    T tmp(std::move(a));    //move a to tmp
    a = std::move(b);       //move b to a
    b = std::move(tmp)      //move tmp to b
}
```

## C++泛型编程概念
- 所谓的泛型编程就是独立于任何特定类型的方式编写代码，使用泛型编程时，需要提供具体程序实例所操作的类型或者值。我们经常使用的STL容器、迭代器都是泛型编程的例子。
- 模板是C++支持参数化多态的工具，使用模板可以使用户为类或者函数声明一种一般模式，使得类中的某些数据成员或者成员函数的参数、返回值取得任意类型。
- 模板是一种对类型进行参数化的工具。通常有两种形式：函数模板和类模板。
- 函数模板针对仅参数类型不同的函数。
- 类模板针对仅数据成员和成员函数类型不同的类。
### 模板函数：
```C++
template <typename T>
int func(const T &a1, const T &a2)
{
    ...
}
template <class T>
inline int func(const T &a1, const T &a2)
{
    ...
}
template<typename T1, typename T2, typename T3>
T1 func(const T2 &t2, const T3 &t3)
{
    ...
}
```
如果类型的定义顺序和调用顺序不一样的话，则需要在申明的时候指定类型顺序：
```C++
template<typename T1, typename T2, typename T3>
T1 func(const T2 &t2, const T3 &t3)
{
    ...
}
func<long, int, long>(12, 34);
```
### 模板类
```C++
template<class Type>
class Queue
{
    ...
    ...
}
// 使用方法
Queue<int> qi;
```

## C++多态的实现及原理
C++多态性用一句话概括就是：在基类的函数前加上virtual关键字，在派生类中重写该函数，运行时会根据对象的实际类型来调用相应的函数，如果对象类型是派生类，就调用派生类的函数；如果对象类型是基类型，就调用基类的函数。这就是C++的多态性。

## 移动构造函数和移动赋值
- 拷贝构造函数
```C++
// 拷贝构造函数
Tracer(Tracer& t)
{
    if (t.text != nullptr) {
        int len = strlen(t.text);
        text = new char[len + 1];
        ctrcpy(text, t.text);
    }
}
```
- 移动构造函数
```C++
Tracer(Tracer&& t)
{
    if (t.text != nullptr)  {
        text = t.text;
        t.text = nullptr;
    }
}
```
- 拷贝赋值函数
```C++
Tracer& operator=(const Tracer& rhs) {
    if (this != &rhs) {
        free();
        if (rhs.text != nullptr) {
            int len = strlen(rhs.text);
            text = new char[len + 1];
            strcpy(text, rhs.text);
        }
    }
    return *this;
}
```
- 移动赋值函数
```C++
Tracer& operator=(const Tracer&& rhs) {
    if (this != &rhs) {
        free();
        text = rhs.text;    // 将当前类的指针指向右值对象的指针
        rhs.text = nullptr; // 将右值对象的指针设为空指针，避免析构函数的调用
    }
}
```

## C++11中的emplace_back()函数
C++11中的emplace_back()函数性能相较于push_back函数好，因为emplace_back()函数直接在vector的尾部进行对象的原地构造，而不需要构造临时对象然后再进行转移构造或者拷贝构造，但是在调用emplace_back()函数时，如果传入的参数是对象的话，还是需要进行拷贝构造或者移动构造，因为传入的时候不可以直接使用参数构造对象，所以这样和push_back无异。所以在调用emplace_back()函数时，传入的参数应该是类的构造函数的参数。