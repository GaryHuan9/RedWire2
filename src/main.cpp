#include "Interface/ApplicationNew.hpp"

int main()
{
	rw::ApplicationNew application;
    while (application.alive()) application.update();
	return 0;
}
