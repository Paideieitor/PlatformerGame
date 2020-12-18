#include "Element.h"

Observer::Observer(Module* module) : module(module), type(Observer::MODULE)
{
	if(!module)
		type = Observer::NONE;
}

Observer::Observer(Entity* entity) : entity(entity), type(Observer::ENTITY)
{
	if (!entity)
		type = Observer::NONE;
}

Observer::Observer(Element* element) : element(element), type(Observer::ELEMENT)
{
	if (!element)
		type = Observer::NONE;
}

Observer::~Observer()
{
}

void* Observer::GetObserver()
{
	switch(type)
	{
	case MODULE:
		return module;
	case ENTITY:
		return entity;
	case ELEMENT:
		return entity;
	}
	return nullptr;
}

Element::Element(const char* name, Type type, fPoint position, iPoint size, Observer* observer)
	: name(name), type(type), position(position), size(size), observer(observer)
{
}

Element::~Element()
{
	if(observer)
		delete observer;
}

void Element::SetPosition(fPoint newPosition) { position = newPosition; }

fPoint Element::GetPosition() { return position; }

void Element::SetSize(iPoint newSize) { size = newSize; }

iPoint Element::GetSize() { return size; }

Element::Type Element::GetType() { return type; }

fPoint Element::GetDrawPosition() { return { position.x - size.x / 2, position.y - size.y / 2 }; }