#include <windows.h>

#include "RenderApplication.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd)
{
	
	RenderApplication window = RenderApplication(hInstance);

	if (!window.Initialize())
	{
		std::cerr << "Window initialization failed" << std::endl;
	}

	window.Run();

	return 0;
}
