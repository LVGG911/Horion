#include "Loader.h"


_Offsets Offsets = _Offsets();

//Entity offsets

int bKillAura;
SlimUtils::SlimMem mem;
const SlimUtils::SlimModule* gameModule;
static bool isRunning = true;

C_LocalPlayer* localPlayer = 0x0;
C_ClientInstance* clientInstance = 0x0;

#if defined _M_X64
#pragma comment(lib, "MinHook.x64.lib")
#elif defined _M_IX86
#pragma comment(lib, "MinHook.x86.lib")
#endif



//Compare the distance when sorting the array of Target Enemies, it's called a "sort predicate"
/*struct CompareTargetEnArray
{
	bool operator() (TargetList_t &lhs, TargetList_t &rhs)
	{
		return lhs.m_Distance < rhs.m_Distance;
	}
};*/


void KillAura()
{
	if (localPlayer == 0x0 || !bKillAura)
		return;
	//Declare our target list to define our victims through a dynamic array
	C_EntityList* entList = localPlayer->getEntityList();
	size_t listSize = entList->getListSize();

	//Loop through all our players and retrieve their information
	float maxDist = 10;
	static std::vector <C_Entity*> targetList;
	targetList.clear();
	for (size_t i = 0; i < listSize; i++)
	{
		C_Entity* currentEntity = entList->get(i);

		if (currentEntity == 0)
			break;

		if (currentEntity == localPlayer) // Skip Local player
			continue;
	
		if ( localPlayer->entityType1 != currentEntity->entityType1 && localPlayer->entityType2 != currentEntity->entityType2) // Skip Invalid Entity
			continue;
		
		float dist = currentEntity->eyePos1.dist(localPlayer->eyePos1);

		if (dist < maxDist) {
			targetList.push_back(currentEntity);
		}
	}

	if(targetList.size() > 0)
		localPlayer->swingArm();

	C_GameMode* gameMode = localPlayer->getCGameMode();

	// Attack all entitys in targetList 
	for (int i = 0; i < targetList.size(); i++)
	{
		gameMode->attack(targetList[i]);
		
	}
}

bool isKeyDown(int key) {
	static uintptr_t keyMapOffset = 0x0;
	if (keyMapOffset == 0x0) {
		uintptr_t sigOffset = Utils::FindSignature("48 8D 0D ?? ?? ?? ?? 89 3C 81 E9");
		if (sigOffset != 0x0) {
			int offset = *reinterpret_cast<int*>((sigOffset + 3)); // Get Offset from code
			keyMapOffset = sigOffset - gameModule->ptrBase + offset + /*length of instruction*/ 7; // Offset is relative
#ifdef _DEBUG
			logF("Recovered KeyMapOffset: %X", keyMapOffset);
#endif
		}
		else
			logF("!!!KeyMap not located!!!");
	}
	// All keys are mapped as bools, though aligned as ints (4 byte)
	// key0 00 00 00 key1 00 00 00 key2 00 00 00 ...
	return *reinterpret_cast<bool*>(gameModule->ptrBase + keyMapOffset + (key * 0x4));
}

bool isKeyPressed(int key) {
	if (isKeyDown(key)) {
		while (isKeyDown(key))
			Sleep(2);
		return true;
	}
	return false;
}

DWORD WINAPI keyThread(LPVOID lpParam)
{
	void* zeHook = 0x0;
	logF("Key thread started");
	while (isRunning) {
		if (isKeyPressed('L')) { // Press L to uninject
			isRunning = false;
			logF("Uninjecting...");
			break;
		}
		if (isKeyPressed('P')) {
			bKillAura = !bKillAura; //true;
			logF("%s KillAura", bKillAura ? "Activating" : "Deactivating");
		}
		
		if (isKeyPressed('O')) {
			localPlayer = clientInstance->getLocalPlayer();
			
			static uintptr_t screenModelBase = 0x0;
			if (screenModelBase == 0x0) {
				uintptr_t sigOffset = Utils::FindSignature("41 89 86 ?? ?? ?? ?? 48 8B 4C 24 ?? 48 89 0D ?? ?? ?? ?? 48 8B 4C 24 ?? 48 89 0D");
				if (sigOffset != 0x0) {
					int offset = *reinterpret_cast<int*>((sigOffset + 15)); // Get Offset from code
					screenModelBase = sigOffset + offset + /*length of instruction*/ 7 + 12; // Offset is relative
				}
				else
					logF("screenModelBase not found!!!");
			}
			uintptr_t* rcx = reinterpret_cast<uintptr_t*>(mem.ReadPtr<uintptr_t>(screenModelBase, { 0, 0x60, 0x10, 0x4B8, 0x0, 0xA8, 0x58, 0x5E0 }) + 0x10); //1.11.0

			C_ClientInstanceScreenModel* cli = reinterpret_cast<C_ClientInstanceScreenModel*>(rcx);
			cli->sendChatMessage("      /\\");
			Sleep(2300);
			cli->sendChatMessage("     /  \\");
			Sleep(2300);
			cli->sendChatMessage("    /    \\");
			Sleep(2300);
			cli->sendChatMessage("   /   @  \\");
			Sleep(2300);
			cli->sendChatMessage("  /_______\\");
		}
		
		if (isKeyPressed('J')) {
			localPlayer = clientInstance->getLocalPlayer();

			if (localPlayer != 0x0) {
				C_MovePlayerPacket* Packet = new C_MovePlayerPacket();
			
				Packet->entityRuntimeID = localPlayer->entityRuntimeId;
				Packet->Position = localPlayer->eyePos0;
				Packet->pitch = localPlayer->pitch;
				Packet->yaw = localPlayer->yaw;
				Packet->headYaw = localPlayer->yaw;
				Packet->onGround = true;
				Packet->mode = 0;

				clientInstance->loopbackPacketSender->sendToServer(Packet);
				delete Packet;
			}
			
		}
		
		if (isKeyPressed('M')) {
			/*logF("Function called");
			localPlayer = clientInstance->getLocalPlayer();
			C_BlockPos* pos = new C_BlockPos();
			pos->x = (int) floorf(localPlayer->eyePos1.x);
			pos->y = 3;
			pos->z = (int)floorf(localPlayer->eyePos1.z);
			logF("Function called2");
			localPlayer->getCGameMode()->_destroyBlockInternal(pos, 2);*/
			zeHook = clientInstance->getLocalPlayer()->getCGameMode()->placeHook(clientInstance);
		}

		if (bKillAura)
		{
			localPlayer = clientInstance->getLocalPlayer();
			if (localPlayer != 0x0) {
				KillAura();
			}
		}
		Sleep(50); // 1000 / 20 
	}
	MH_DisableHook(zeHook);

	FreeLibraryAndExitThread(static_cast<HMODULE>(lpParam), 1); // Uninject
}

DWORD WINAPI startCheat(LPVOID lpParam)
{
	logF("Starting cheat...");
	DWORD procId = GetCurrentProcessId();
	if (!mem.Open(procId, SlimUtils::ProcessAccess::Full))
	{
		logF("Failed to open process, error-code: %i", GetLastError());
		return 1;
	}
	gameModule = mem.GetModule(L"Minecraft.Windows.exe"); // Get Module for Base Address
	


	logF("MH_Initialize %i", MH_Initialize());
	//clientInstance = mem.ReadPtr<C_ClientInstance*>(gameModule->ptrBase + 0x26dc038, { 0x0, 0x10, 0xF0, 0x0 }); //1.11.0
	clientInstance = mem.ReadPtr<C_ClientInstance*>(gameModule->ptrBase + 0x26e1108, { 0x0, 0x10, 0xF0, 0x0 });   //1.11.1

	logF("Starting threads...");

	CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)keyThread, lpParam, NULL, NULL); // Checking Keypresses
	logF("Started!");

	ExitThread(0);
}

BOOL __stdcall
DllMain(HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID
)
{

	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH: //When the injector is called.
	{
		logF("Starting cheat");

		CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)startCheat, hModule, NULL, NULL);
		DisableThreadLibraryCalls(hModule);
	}
	break;
	case DLL_PROCESS_DETACH:
		logF("Removing logger");
		DisableLogger();
		MH_Uninitialize();
		break;
	}
	return TRUE;
}