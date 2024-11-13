#include <iostream>
#include "../include/SkipList.h"
#include <string>

using namespace std;
using namespace ty;

void save(std::ofstream &fp, std::string k, std::string v)
{
    fp << k << ":" << v << std::endl;
}

bool load(std::ifstream &fp, std::string& k, std::string& v)
{
    std::string line;
    getline(fp, line);
    int pos = line.find(":");
    if(pos == string::npos){
        return false;
    }
    k = line.substr(0, pos);
    v = line.substr(pos + 1);
    return true;
}

int main()
{
    SkipList<string, string> list;
    list.insert("a", "你是什么?");
    list.insert("b", "大家好!!!");
    list.insert("哈哈哈哈哈", "耶耶耶耶耶耶");
    list.insert("66大顺", "恭喜发财");
    string ret;
    list.select("a", &ret);
    cout << "select a:" << ret << endl;
    list.select("66大顺", &ret);
    cout << "select 66大顺:" << ret << endl;
    list.select("b", &ret);
    cout << "select b:" << ret << endl;

    cout << "list:" << endl;
    for (size_t i = 0; i < list.size(); i++)
    {
        cout << list.at(i) << endl;
    }

    list.save("./file.dat", save);

    SkipList<string, string> list2;
    cout << "load file" << endl;
    list2.load("./file.dat", load);
    cout << "list2:" << endl;
    for (size_t i = 0; i < list2.size(); i++)
    {
        cout << list2.at(i) << endl;
    }
}