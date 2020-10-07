#include "App.h"
#include "Window.h"
#include "Input.h"
#include "Render.h"
#include "Textures.h"
#include "Audio.h"
#include "Scene.h"

#include "Defs.h"
#include "Log.h"

#include <iostream>
#include <sstream>

// Constructor
App::App(int argc, char* args[]) : argc(argc), args(args)
{
	frames = 0;

	configPath = "config.xml";
	savePath = "savegame.xml";
	toSave = false;
	toLoad = false;

	input = new Input();
	win = new Window();
	render = new Render();
	tex = new Textures();
	audio = new Audio();
	scene = new Scene();

	// Ordered for awake / Start / Update
	// Reverse order of CleanUp
	AddModule(input);
	AddModule(win);
	AddModule(tex);
	AddModule(audio);
	AddModule(scene);

	// render last to swap buffer
	AddModule(render);
}

// Destructor
App::~App()
{
	// release modules
	while(modules.size() != 0)
	{
		delete* modules.begin();
		modules.erase(modules.begin());
	}

	configFile.reset();
}

void App::AddModule(Module* module)
{
	module->Init();
	modules.emplace_back(module);
}

// Called before render is available
bool App::Awake()
{
	if (!LoadConfig())
		return false;

	Module* pModule = NULL;

	title = configApp.child("title").child_value();
	win->SetTitle(title.c_str());

	for(std::vector<Module*>::iterator m = modules.begin(); m != modules.end(); m++)
	{
		pModule = *m;

		if(!pModule->Awake(config.child(pModule->name.c_str())))
			return false;
	}

	return true;
}

// Called before the first frame
bool App::Start()
{
	Module* pModule = NULL;

	for(std::vector<Module*>::iterator m = modules.begin(); m != modules.end(); m++)
	{
		pModule = *m;

		if(!pModule->Start())
			return false;
	}

	return true;
}

// Called each loop iteration
bool App::Update()
{
	PrepareUpdate();

	if(input->GetWindowEvent(WE_QUIT) == true)
		return false;

	if(!PreUpdate())
		return false;

	if(!DoUpdate())
		return false;

	if(!PostUpdate())
		return false;

	FinishUpdate();
	return true;
}

bool App::LoadConfig()
{
	pugi::xml_parse_result result = configFile.load_file(configPath.c_str());

	if(result == NULL)
	{
		LOG("Could not load map xml file %s. pugi error: %s", configPath.c_str(), result.description());
		return false;
	}
	else
	{
		config = configFile.child("config");
		configApp = config.child("app");
	}

	return true;
}

// ---------------------------------------------
void App::PrepareUpdate()
{
}

// ---------------------------------------------
void App::FinishUpdate()
{
	if(input->GetKey(SDL_SCANCODE_S) == KEY_DOWN)
		toSave = true;
	if(input->GetKey(SDL_SCANCODE_L) == KEY_DOWN)
		toLoad = true;

	if(toSave)
	{
		SaveGame();
		toSave = false;
	}

	if(toLoad)
	{
		LoadGame();
		toLoad = false;
	}
}

// Call modules to save data in the save file
bool App::SaveGame()
{
	Module* pModule = NULL;

	pugi::xml_document saveDoc;
	pugi::xml_parse_result result = saveDoc.load_file(savePath.c_str());

	for(std::vector<Module*>::iterator m = modules.begin(); m != modules.end(); m++)
	{
		pModule = *m;

		pugi::xml_node mNode = saveDoc.child("save").child(pModule->name.c_str());
		if(mNode == NULL)
			mNode = saveDoc.child("save").append_child(pModule->name.c_str());

		if(!pModule->Save(mNode))
		{
			LOG("Saving Error in %s", pModule->name.c_str());
			return false;
		}
		saveDoc.save_file(savePath.c_str());
	}

	return true;
}

// Call modules to load data from the save file
bool App::LoadGame()
{
	Module* pModule = NULL;

	pugi::xml_document saveDoc;
	pugi::xml_parse_result result = saveDoc.load_file(savePath.c_str());

	for(std::vector<Module*>::iterator m = modules.begin(); m != modules.end(); m++)
	{
		pModule = *m;

		pugi::xml_node mNode = saveDoc.child("save").child(pModule->name.c_str());
		if(mNode == NULL)
			return false;

		if(!pModule->Load(mNode))
		{
			LOG("Loading Error in %s", pModule->name.c_str());
			return false;
		}
	}

	return true;
}

// Call modules before each loop iteration
bool App::PreUpdate()
{
	Module* pModule = NULL;

	for(std::vector<Module*>::iterator m = modules.begin(); m != modules.end(); m++)
	{
		pModule = *m;

		if(pModule->active == false)
			continue;

		if(!pModule->PreUpdate())
			return false;
	}

	return true;
}

// Call modules on each loop iteration
bool App::DoUpdate()
{
	Module* pModule = NULL;

	for(std::vector<Module*>::iterator m = modules.begin(); m != modules.end(); m++)
	{
		pModule = *m;

		if(pModule->active == false)
			continue;

		if(!pModule->Update(dt))
			return false;
	}

	return true;
}

// Call modules after each loop iteration
bool App::PostUpdate()
{
	Module* pModule = NULL;

	for (std::vector<Module*>::iterator m = modules.begin(); m != modules.end(); m++)
	{
		pModule = *m;

		if(pModule->active == false)
			continue;

		if(!pModule->PostUpdate())
			return false;
	}

	return true;
}

// Called before quitting
bool App::CleanUp()
{
	Module* pModule = NULL;

	for(std::vector<Module*>::iterator m = modules.end() - 1; m != modules.begin(); m--)
	{
		pModule = *m;

		if (!pModule->CleanUp())
			return false;
	}

	return true;
}

// ---------------------------------------
int App::GetArgc() const
{
	return argc;
}

// ---------------------------------------
const char* App::GetArgv(int index) const
{
	if(index < argc)
		return args[index];
	else
		return NULL;
}

// ---------------------------------------
const char* App::GetTitle() const
{
	return title.c_str();
}

// ---------------------------------------
const char* App::GetOrganization() const
{
	return organization.c_str();
}

pugi::xml_document* App::GetConfig(pugi::xml_node& node)
{
	node = config;
	return &configFile;
}