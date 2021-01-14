#include <g.h>


struct my_core : public g::core
{
	virtual bool initialize()
	{
		std::cout << "initialize your game state here.\n";
	
		return true;
	}

	virtual void update(float dt)
	{
		if (counter > 0)
		{
			std::cout << counter << std::endl;
			counter--;			
		}
 		else
 		{
 			std::cout << counter << " blast-off!\n";
 			running = false;
 		}
	}

	int counter = 10;
};


int main (int argc, const char* argv[])
{
	my_core count_down;

	count_down.start({});

	return 0;
}