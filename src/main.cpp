#include "Interface/Application/Application.hpp"

int main()
{
	rw::Application application;
    while (application.alive()) application.update();
	return 0;
}
