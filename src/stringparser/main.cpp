#include <iostream>
#include <sstream>
#include <set>
#include <map>

#include <sys/stat.h>

using namespace std;

extern "C" int execute(char *mod_name_amx, const char* mod_name);

extern "C" void func_CreateDynamicObject(char* name, FILE* _fpfunc);
extern "C" void func_CreateObject(char* name, FILE* _fpfunc);
extern "C" void func_Create3DTextLabel(char* name, FILE* _fpfunc);

map <string, void(*)(char*, FILE* _fpfunc)> function_list = {
        { "CreateDynamicObject", func_CreateDynamicObject },
        { "CreateObject", func_CreateObject },
        { "Create3DTextLabel", func_Create3DTextLabel }
};

#define size_functions 3 // Количество функций

FILE *fpfunc[size_functions];

int getCountKeyFromMap(string name);

extern "C" void api(char* name)
{
    string key;
    stringstream ss;
    ss << name;
    ss >> key;
    if(function_list.count(key)) function_list[key](name, fpfunc[getCountKeyFromMap(key)]);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        cout << "Usage: stringparser [*.amx]" << endl;
        return 1;
    }

    string _func, mod_name = argv[1];
    mod_name.resize(mod_name.size() - 4);
    int i = 0;

    mkdir(mod_name.c_str(), 0777);

    for(auto& it : function_list)
    {
        _func = mod_name + "/func_" + it.first + "_" + mod_name + ".txt";
        if ((fpfunc[i] = fopen(_func.c_str(), "wt")) == NULL)
        {
            printf("Unable to create output file \"%s\"\n", it.first);
            return 1;
        }
        i++;
    }

    execute(argv[1], mod_name.c_str());

    for(int i = 0; i < size_functions; i++)
        fclose(fpfunc[i]);
    return 0;
}

int getCountKeyFromMap(string name)
{
    string key;
    stringstream ss;
    ss << name;
    ss >> key;
    int i = 0;
    for(auto& it : function_list)
    {
        if(key == it.first)
            return i;
        i++;
    }
    return -1;
}