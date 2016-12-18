#pragma once

#include <iostream>
#include <string>
#include "md5.h"

using namespace std;

void printEmpty(int count = 1);
void printDivisor(string& header = string(), int width = 80);

string utf8_encode(string str);
string utf8_encode(const wstring &wstr);
wstring utf8_decode(const string &str);

void XOR(char * data, unsigned long size, const char * password, int passwordSize);
