# cpy 

关于C和Python之间相互调用的代码笔记

---

## C++调用python

### cpp端 "cpp_call_py.cpp"

测试环境：win10 + VS2017 + python 3.8.10 + numpy 1.18.1

添加Python相关头文件

```cpp
#include <Python.h>
#include <numpy/arrayobject.h>
#include <frameobject.h> 
```

头文件，如：Python38\include\;Python38\Lib\site-packages\numpy\core\include\
           
lib文件，如：Python38\libs\python38.lib

dll文件，如：Python38\python38.dll

具体示例源码：cpp_call_py.cpp

demo其他依赖：OpenCV

### python端 "cpp_call_py.py"

python环境正常即可

## Python调用C