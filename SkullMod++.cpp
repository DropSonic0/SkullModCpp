// SkullMod++.cpp : This file contains the 'main' function. Program execution begins and ends there.

#include <iostream>
#include <Windows.h>
#include <string.h>
#include <string>
#include "gfs.h"

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
    GFSUnpacker GFSUnpack;
    GFSPacker GFSpack;
    for (int i{ 1 }; i < argc; i++) {
        std::filesystem::path fileread = argv[i];
        std::cout << "File Read Path:" << fileread << '\n';
        //GFS gfs(fileread);
        
        if (fileread.extension() == "") {
            GFSpack(fileread);
        }
        else {
            GFSUnpack(fileread);
        }
    } //for
} //main

int test() {
	GFSPacker GFSPack;
	GFSPack("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Skullgirls\\data01\\core");
}