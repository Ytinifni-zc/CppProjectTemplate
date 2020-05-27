//
// Created by inFinity on 2019-08-26.
//

#ifndef KALEIDO_SYSCMD_H
#define KALEIDO_SYSCMD_H

#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>

auto exeCmd(const std::string& cmd) {
    auto fp = popen(cmd.data(), "r");
    if (!fp) {
        std::cerr << "Error commend: " << cmd << std::endl;
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


#endif //KALEIDO_SYSCMD_H
