#pragma once
#include "Module.h"
class Derp :
	public IModule
{
private:
	int c = 0;
public:
	C_ChestScreenController* chestScreenController = nullptr;
	Derp();
	~Derp();

	// Inherited via IModule
	virtual void onTick(C_GameMode* gm) override;
	virtual const char* getModuleName() override;
};