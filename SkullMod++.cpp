// SkullMod++.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <iostream>
#include <Windows.h>
#include <string.h>
#include <string>
#include <algorithm>
#include <vector>
#include "gfs.h"
#include "gbs.h"

//-----------------------

int main(int argc, char* argv[])
{
    printf("   _____   _              _   _   __  __               _                 \n");
    printf("  / ____| | |            | | | | |  \\/  |             | |    _       _   \n");
    printf(" | (___   | | __  _   _  | | | | | \\  / |   ___     __| |  _| |_   _| |_ \n");
    printf("  \\___ \\  | |/ / | | | | | | | | | |\\/| |  / _ \\   / _` | |_   _| |_   _|\n");
    printf("  ____) | |   <  | |_| | | | | | | |  | | | (_) | | (_| |   |_|     |_|  \n");
    printf(" |_____/  |_|\\_\\  \\__,_| |_| |_| |_|  |_|  \\___/   \\____|                \n\n");
    printf("Version: 0.3 \n");
    printf("Original program made by 0xFAIL\n");
    printf("The C++ version is made by ImpDi\n\n");
    if (argc == 1) {
        std::cout << "There are no files" << '\n';
        return 0;
    }

    bool is_ps3 = false;
    std::vector<std::string> files;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.size() >= 2 && arg.front() == '"' && arg.back() == '"') {
            arg = arg.substr(1, arg.size() - 2);
        }
        if (arg == "-ps3") {
            is_ps3 = true;
        }
        else {
            files.push_back(arg);
        }
    }

    GFSUnpacker GFSUnpack;
    GFSPacker GFSpack;
    for (const auto& file_path : files) {
        std::filesystem::path fileread = file_path;
        std::cout << "File Read Path:" << fileread << '\n';
        
        std::string ext = fileread.extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

        if (ext == ".gfs") {
            GFSUnpack(fileread);
        }
        else if (ext == ".gbs") {
            try {
                gbs::gbs_t gbs_file(fileread, is_ps3);
                std::filesystem::path filewrite = fileread;
                if (is_ps3) {
                    filewrite.replace_filename(fileread.stem().string() + "_ps3.gbs");
                }
                else {
                    filewrite.replace_filename(fileread.stem().string() + "_pc.gbs");
                }
                gbs_file.write(filewrite);
                std::cout << "GBS processed: " << filewrite << "\n";
            }
            catch (const std::exception& e) {
                std::cerr << "Error processing GBS: " << e.what() << "\n";
            }
        }
        else if (ext == "") {
            GFSpack(fileread);
        }
        else {
            std::cout << "Unsupported extension: " << ext << "\n";
        }
    } //for
} //main

int test() {
	GFSPacker GFSPack;
	GFSPack("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Skullgirls\\data01\\core");
}