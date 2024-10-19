#include "RPGServer.hpp"
#include <conio.h>

const int gBindPort = 12345;
const unsigned short gMaxClient = 100;

int main()
{
	RPGServer server(gBindPort, gMaxClient);
	server.Start();
	std::cout << "아무키나 누르면 종료됩니다.\n";
	_getch();

	server.End();

}