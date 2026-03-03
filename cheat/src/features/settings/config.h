#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <map>
#include "../../globals.h"

namespace config
{
    void init();
    void save(const std::string& name);
    void load(const std::string& name);
    void remove(const std::string& name);
    void refresh();
    void reset();
    void set_auto_load(const std::string& name);
    void load_auto_config();

    inline std::vector<std::string> list;
    inline std::string current_config = "";
    inline char name_buffer[64] = "";
}

