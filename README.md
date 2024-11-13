## KV存储引擎

使用跳表实现的KV存储引擎

支持 时间复杂度 log(n) 的随机访问

示例文件 ./test/test.cpp

## 接口 

* insert(K k, V v) 插入数据
* select(K k, V* v) 搜索k的结果保存到v
* erase(K k) 删除数据
* at(size_t index) 获取第 index 个数据 
* size() 返回数据大小
* save(const char *file) 保存数据
* save(const char *file, std::function<void(std::ofstream &fp, K, V)> func) 以自定义的方法保存数据
* load(const char *file) 加载数据
* load(const char *file, std::function<bool(std::ifstream &fp, K&, V&)> func) 以自定义的方法加载数据