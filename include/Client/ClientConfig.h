#pragma once

#include <string>
#include <fstream>
#include <algorithm>

using namespace std;

class ClientConfig {
public:
	string addr;
	string login;
	string password;
	string workdir;
	//Start symbol in server path
	int ssisp;

	ClientConfig(string& addr, string& login, string& password, string& workdir) : addr(addr), login(login), password(password), workdir(workdir) {
		int wdpsize = workdir.size();
		ssisp = wdpsize == 0 ? 0 : wdpsize + 1;
	}

	string serverPathFromLocal(string& localPath) {
		string result = ssisp < localPath.size() ? localPath.substr(ssisp) : "";
		replace(result.begin(), result.end(), '\\', '/');
		return result;
	}

	string localPathFromServer(string& serverPath) {
		string result = workdir + "\\" + serverPath;
		replace(result.begin(), result.end(), '/', '\\');
		return result;
	}

	static ClientConfig read(string& path) {
		ifstream confStream(path);
		string addr, login, password, workdir;
		std::getline(confStream, addr);
		std::getline(confStream, login);
		std::getline(confStream, password);
		std::getline(confStream, workdir);

		if (workdir.empty())
			workdir = login;
		confStream.close();
		return ClientConfig(addr, login, password, workdir);
	}
};