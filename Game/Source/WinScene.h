#ifndef __WINSCENE_H__
#define __WINSCENE_H__

#include "Module.h"

struct SDL_Texture;

class WinScene : public Module
{
public:

	WinScene();
	virtual ~WinScene();

	bool Awake(pugi::xml_node&);
	bool Start();

	bool PreUpdate();
	bool Update(float dt);
	bool PostUpdate();

	bool CleanUp();

private:

	SDL_Texture* win;
};

#endif //__WINSCENE_H__