#pragma once

#include <iostream>
#include <string>

using namespace std;

void printEmpty(int count = 1);
void printDivisor(string& header = string(), int width = 80);

wchar_t * toWCStr(const char * str);
char * toCStr(const wchar_t * wcstr);

string toString(wstring& wstr);
wstring toWString(string& str);

string utf8_encode(string str);
string utf8_encode(const wstring &wstr);
wstring utf8_decode(const string &str);


void XOR(char * data, unsigned long size, const char * password, int passwordSize);
