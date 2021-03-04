//
// Created by inFinity on 2020/5/27.
//

#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>
#include <iostream>
#include <fstream>
#include <random>

#include <utils/Log.h>

auto&& exeCmd(const std::string& cmd) {
    auto fp = popen(cmd.data(), "r");
    if (!fp) {
        LOG::error("Error commend: {}", cmd);
        exit(1);
    }
    char line[1024];
    std::stringstream ss("");
    while (fgets(line, sizeof(line)-1, fp) != nullptr) {
        if (strcmp(line, "")==0) break;
        ss << line;
    }
    pclose(fp);
    return std::move(ss);
}

int main(int argc, char *argv[]) {
    int up = 100000000;
    if (argc>1) up = atoi(argv[1]);
    std::ofstream ofs("random.txt", std::ofstream::binary);
    for (auto i = 0u; i < up; ++i) {
        int r = std::rand() % up;
        ofs.write((char*)&r, sizeof(r));
    }
    ofs.close();
    return 0;
}
