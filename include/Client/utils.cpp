#include "utils.h"

#include <windows.h>


void printEmpty(int count) {
	for (int counter = 0; counter < count; counter++)
		cout << endl;
}

void printDivisor(string& header, int width) {
	const char symbol = '-';
	int headerSize = header.size();
	int beginWidth = (width - headerSize) / 2;
	for (int counter = 0; counter < beginWidth; counter++)
		cout << symbol;
	cout << header;
	int endWidth = width - beginWidth - headerSize;
	for (int counter = 0; counter < endWidth; counter++)
		cout << symbol;
	cout << endl;
}

wchar_t * toWCStr(const char * str) {
	int strLength = strlen(str);
	wchar_t * wcstr = new wchar_t[strLength + 1];
	MultiByteToWideChar(CP_ACP, 0, str, strLength + 1, wcstr, strLength + 1);
	return wcstr;
}

char * toCStr(const wchar_t * wcstr) {
	int strLength = wcslen(wcstr);
	char * cstr = new char[strLength + 1];
	WideCharToMultiByte(CP_ACP, 0, wcstr, strLength + 1, cstr, strLength + 1, 0, 0);
	return cstr;
}

string utf8_encode(string str) {
	return utf8_encode(wstring(toWCStr(str.c_str())));
}

string utf8_encode(const wstring &wstr) {
	if (wstr.empty()) return string();
	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
	string strTo(size_needed, 0);
	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
	return strTo;
}

wstring utf8_decode(const string &str) {
	if (str.empty()) return wstring();
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
	wstring wstrTo(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
	return wstrTo;
}

string toString(wstring& wstr) {
	char * cstr = toCStr(wstr.c_str());
	string result(cstr);
	//delete[] cstr;
	return result;
}

wstring toWString(string& str) {
	wchar_t * wstr = toWCStr(str.c_str());
	wstring result(wstr);
	delete[] wstr;
	return result;
}

void XOR(char * data, unsigned long size, const char * password, int passwordSize) {
	for (unsigned long index = 0; index < size; index++) {
		data[index] ^= password[index % passwordSize];
	}
}