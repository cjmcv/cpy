#include <iostream>
#include <string>

#include "opencv2/opencv.hpp"

#include <Python.h>
#include <numpy/arrayobject.h>
#include <frameobject.h>

class PyManager {
public:
  // 环境初始化与模块导入
  bool Init(std::string module_name) {
    if (Py_IsInitialized()) {
      printf("Warning: Py_Initialize has been called.\n");
    }
    else {
      Py_Initialize(); // 启动虚拟机
    }
    
    if (!Py_IsInitialized()) {
      printf("Py_Initialize Failed.\n");
      return false;
    }
    import_array();  // numpy
    
    // 导入模块
    module_ = nullptr;
    module_ = PyImport_ImportModule(module_name.c_str());// Python文件名
    if (!module_) {
      printf("Can not import module: %s.\n", module_name.c_str());
      return false;
    }
    
    // 获取导入模块中的方法字典
    func_dict_ = nullptr;
    func_dict_ = PyModule_GetDict(module_);
    if (!func_dict_) {
      printf("Cant find dictionary./n");
      return false;
    }
    return true;
  }
  
  // 反初始化
  void UnInit() {
    Py_DECREF(module_);
    Py_Finalize(); // 关闭虚拟机
  }
  
  // 转换opencv图像数据用于输入
  PyObject *BuildImageData(cv::Mat img) {
    int height, width;
    width = img.cols * img.channels();
    height = img.rows;
    npy_intp Dims[2] = { height, width }; //给定维度信息
    NPY_TYPES type;
    
    if (img.depth() == CV_8U)
      type = NPY_UBYTE;
    else if (img.depth() == CV_32F)
      type = NPY_FLOAT;
    else {
      printf("Error: img.depth() = %d is not supported.\n", img.depth());
      exit(100);
    }
    // 是否需要手动释放？
    return PyArray_SimpleNewFromData(2, Dims, type, img.data);
  }
  
  // 构建结构体
  template <typename T>
  inline PyObject *BuildStruct(std::string format, int alignment, T &struct_val) {
    // Python3中有bytes类型"y"，不能与string "s"混淆.
    // python2中可用"s#"代替"y#".
    std::string final_format;
    switch (alignment) {
    //case 1:
    //  final_format = format + "0b"; // 1和2对齐不吻合？
    //  break;
    //case 2:
    //  final_format = format + "0h";
    //  break;
    case 4:
      final_format = format + "0l";
      break;
    case 8:
      final_format = format + "0q";
      break;
    default:
      printf("Error: Alignment is only support 4/8 -> (%d).\n", alignment);
      final_format = format;
    }
    return Py_BuildValue("sy#", final_format, &struct_val, sizeof(T));
  }
  
  // 调用python函数
  bool Call(std::string func_name, PyObject *input = nullptr, PyObject **ret = nullptr) {
    PyObject*func = PyDict_GetItemString(func_dict_, func_name.c_str());
    if (ret == nullptr) {
      PyObject_CallObject(func, input);
    }
    else {
      *ret = PyObject_CallObject(func, input);
    }
    return true;
  }
  
  // 解析返回的list值
  template <typename T>
  bool ParseList(PyObject *ret, std::string type, std::vector<std::vector<T>> &parsed_ret) {
    if (!PyList_Check(ret)) {
      return false;
    }
    int size = PyList_Size(ret);
    parsed_ret.resize(size);
    for (int i = 0; i < size; i++) {
      PyObject *items = PyList_GetItem(ret, i);
      int num = PyList_Size(items);
      for (int j = 0; j < num; j++) {
        PyObject *item = PyList_GetItem(items, j);
        T t;
        if (!PyArg_Parse(item, type.c_str(), &t)) {
          LogException();
          return false;
        }
        parsed_ret[i].push_back(t);
      }
    }
    return true;
  }
  
  // 如有异常，输出异常信息
  void LogException() {
    if (!Py_IsInitialized()) {
      std::cout << "Python 运行环境没有初始化！";
      return;
    }
    // 检查错误指示器是否被设置，即检查是否有出现错误
    if (PyErr_Occurred() == NULL) {
      return;
    }
    
    //err_msg_.clear();
    //err_msg_.str("");
    // 获取错误信息。
    PyObject *type, *value, *traceback;
    PyErr_Fetch(&type, &value, &traceback);
    PyErr_NormalizeException(&type, &value, &traceback);
    
    if (type) {
      std::cout << PyExceptionClass_Name(type) << "->"; // const char* err_type
    }
    
    if (value) {
      PyObject *line = PyObject_Str(value);
      if (line && (PyUnicode_Check(line))) {
        std::cout << PyUnicode_AsUTF8(line); // const char* err_msg
                                      //std::cout << PyUnicode_1BYTE_DATA(line);
      }
      Py_DECREF(line);
    }
    
    // 在py文件里的错误会有traceback；如果只是cpp端有误，则没有。
    if (traceback) {
      for (PyTracebackObject *tb = (PyTracebackObject *)traceback;
        tb != NULL;
        tb = tb->tb_next) {
        PyObject *line = PyUnicode_FromFormat("  File \"%U\", line %d, in %U\n",
          tb->tb_frame->f_code->co_filename,
          tb->tb_lineno,
          tb->tb_frame->f_code->co_name);
        std::cout << PyUnicode_1BYTE_DATA(line);
      }
    }
    
    //std::cout << "Error: " << err_msg_.str() << std::endl;
    Py_XDECREF(type);
    Py_XDECREF(value);
    Py_XDECREF(traceback);
  }
  
private:
  //std::stringstream err_msg_;
  PyObject *module_;
  PyObject *func_dict_;
};

// https://docs.python.org/3/library/struct.html
// note: 1、结构体内只支持 C 基础类型
//       2、python解析时，需要在末尾添加"0X"声明对齐方式，
//       如X为i时，表示以int作对齐，即4字节对齐。d为8字节对齐。
//       3、ValueError->embedded null byte???
//       If the string is shorter than count-1, it is padded with
//       null bytes so that exactly count bytes in all are used.
#pragma pack(4)
typedef struct _TestStructA {
  int a;      // i
  float b;    // f
  char c[8];  // 8s
  double d;   // d
  short e;    // h
  int f[2];   // ii / 2i
}TestStruct;

int main() {
  PyManager pm;
  pm.Init("cpp_call_py");
  
  while(1) {
    for (int ci = 0; ci < 2; ci++) {
      // Test image.
      printf("---------- Test image. -----------\n");
      {
        cv::Mat img = cv::imread("test.jpg", ci);
        //
        PyObject *args = PyTuple_New(4);
        PyTuple_SetItem(args, 0, pm.BuildImageData(img));
        PyTuple_SetItem(args, 1, Py_BuildValue("i", img.channels())); // s/z/i/l/c/d/D/O/S
        PyTuple_SetItem(args, 2, Py_BuildValue("s", "2.jpg"));
        PyTuple_SetItem(args, 3, Py_BuildValue("ii", 4, 5));
        PyObject *ret_val;
        pm.Call("load_image", args, &ret_val);
        Py_DECREF(args);
        // Test ParseList
        std::vector<std::vector<float>> func_ret;
        pm.ParseList(ret_val, "f", func_ret);
        for (int i = 0; i < func_ret.size(); i++) {
          for (int j = 0; j < func_ret[i].size(); j++) {
            printf("%f, ", func_ret[i][j]);
          }
          printf("\n");
        }
      }
      
      // Test ParseTuple & ParseList
      printf("---------- Test ParseTuple & ParseList. -----------\n");
      {
        std::vector<std::vector<float>> func_ret;
        PyObject *ret_obj;
        pm.Call("get_tuple", NULL, &ret_obj);
        char *str1, *str2;
        int val;
        PyObject *list = nullptr;
        PyArg_ParseTuple(ret_obj, "sisO", &str1, &val, &str2, &list);
        func_ret.clear();
        pm.ParseList(list, "f", func_ret);
        for (int i = 0; i < func_ret.size(); i++) {
          for (int j = 0; j < func_ret[i].size(); j++) {
            printf("%f, ", func_ret[i][j]);
          }
          printf("\n");
        }
      }
      
      // Test tracebackLogException.
      printf("---------- Test tracebackLogException. -----------\n");
      {
        pm.Call("test_traceback", NULL, NULL);
        pm.LogException();
      }
      
      // Test struct
      printf("---------- Test struct. -----------\n");
      {
        printf("sizeof(TestStruct): %d.\n", sizeof(TestStruct));
        TestStruct input;
        input.a = 1;
        input.b = 2.2;
        for (int i = 0; i < 8; i++) {
          input.c[i] = 'a';
        }
        input.d = 3.45;
        input.e = 6;
        for (int i = 0; i < 2; i++) {
          input.f[i] = i;
        }
        // 输入和运行
        PyObject *args = PyTuple_New(1);
        PyTuple_SetItem(args, 0, pm.BuildStruct("if8sdh2i", 4, input));
        PyObject *ret_obj;
        pm.Call("test_struct", args, &ret_obj);
        Py_DECREF(args);
        pm.LogException();
        //获取返回值
        TestStruct* ts_ret;
        PyArg_Parse(ret_obj, "y", &ts_ret);
        pm.LogException();
        //转成结构体
        printf("\nC++ receive: %d, %f, %f, %d, %d, %d\n",
          ts_ret->a, ts_ret->b, ts_ret->d, ts_ret->e, ts_ret->f[0], ts_ret->f[1]);
        printf("var c list = ");
        for (int i = 0; i < 8; i++) {
          printf("%c, ", ts_ret->c[i]);
        }
      }
      printf("\n>>\n>>\n>>\n>>\n>>\n");
    }
  }
  //
  pm.UnInit();
  system("pause");
}