#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <deque>
#include <set>
#include <map>

using namespace std;

extern "C" int execute(int argc,char *argv[]);
extern "C" void func_CreateDynamicObject(char* name);
extern "C" void func_CreateObject(char* name);
extern "C" void func_Create3DTextLabel(char* name);

map <string, void(*)(char*)> function_list = {
        { "CreateDynamicObject", func_CreateDynamicObject },
        { "CreateObject", func_CreateObject },
        { "Create3DTextLabel", func_Create3DTextLabel }
};

extern "C" void api(char* name)
{
    string key;
    stringstream ss;
    ss << name;
    ss >> key;
    if(function_list.count(key)) function_list[key](name);
}

int main(int argc, char *argv[])
{
    execute(argc, argv);
    return 0;
}