#include "library.h"
#include "mod/amlmod.h"
#include "mod/config.h"
#include "mod/logger.h"
#include "GTASA/common.h"
#include "shared/CEvents.h"
#include "shared/ScriptCommands.h"
#include "GTASA/CTimer.h"
#include "GTASA/CPools.h"
#include "GTASA/CCamera.h"
#include "funcs/Panda.h"
#include "funcs/Shadow.h"
#include "GlossHook/include/Gloss.h"
#include <iostream>
#include <cstdlib>
#include <ctime>

#define HOOKBW(_name, _fnAddr)                                    \
	CPatch::TrampolinesRedirectCall(SET_THUMB, _fnAddr, (void*)(&HookOf_##_name), (void**)(&_name), BW_THUMB32);//6A //0x004A69A6
    
MYMOD(CarDoorCollision, GTASA, 1.0, Panda)
NEEDGAME(com.rockstargames.gtasa)

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <cmath>
// Global Variables
const float distX = 0.7f;
const float distY = 0.2f;
std::unordered_map<std::string, int> colls;
std::vector<std::pair<int, int>> objects;

using namespace plugin;
RwV3d localComponentOffsetPos(RwFrame *component) {
    RwV3d pos;
    if (component) {
        pos = component->modelling.pos;
        pos.z += 0.50f;
    }
    return pos;
}

RwFrame* rpClumpFindFrameById(CVehicle* vehicle, int id) {
    if (vehicle && id > 0) {
        RpClump* clump = vehicle->m_pRwClump; // Get RpClump pointer from the vehicle
        if (clump) {
            RwFrame* frame = CClumpModelInfo::GetFrameFromId(clump, id);
            if (frame) {
                return frame;
            }
        }
    }
    return nullptr;
}

int newObject(int vehicle) {
    int id = 914;
    if (!Command<Commands::HAS_MODEL_LOADED>(id)) {
        Command<Commands::REQUEST_MODEL>(id);
        Command<Commands::LOAD_ALL_MODELS_NOW>();
    }
    int object;
    Command<Commands::CREATE_OBJECT>(id, 0.0f, 0.0f, 0.0f, &object);
    Command<Commands::SET_OBJECT_VISIBLE>(object, false);
    objects.emplace_back(object, vehicle);
    return object;
}

void clearObjects() {
    for (const auto& item : objects) {
        if (Command<Commands::DOES_OBJECT_EXIST>(item.first)) {
            Command<Commands::DELETE_OBJECT>(item.first);
        }
    }
    objects.clear();
}

void removeObjectsByVehicle() {
    objects.erase(std::remove_if(objects.begin(), objects.end(), [](const auto& item) {
        if (!Command<Commands::DOES_VEHICLE_EXIST>(item.second)) {
            if (Command<Commands::DOES_OBJECT_EXIST>(item.first)) {
                Command<Commands::DELETE_OBJECT>(item.first);
            }
            return true;
        }
        return false;
    }), objects.end());
}

void DoorCollision(CVehicle *veh) {
       /* for (int i = 0; i < CPools::ms_pVehiclePool->m_nSize; i++) {
            CVehicle *veh = CPools::ms_pVehiclePool->GetAt(i);*/
            int vehicle = CPools::GetVehicleRef(veh);
            float speed = veh->m_vecMoveSpeed.Magnitude();
            if (speed <= 1.0f) {
                RwFrame *Door_lf_dummy = rpClumpFindFrameById(veh, 10);
                std::string colL = "L" + std::to_string(veh->m_nModelIndex);
                if (Door_lf_dummy && Command<Commands::IS_CAR_DOOR_FULLY_OPEN>(vehicle, 2)) {
                    CVector doorPos = localComponentOffsetPos(Door_lf_dummy);
                    if (doorPos.x != 0 || doorPos.y != 0 || doorPos.z != 0) {
                        if (!Command<Commands::DOES_OBJECT_EXIST>(colls[colL])) {
                            colls[colL] = newObject(vehicle);
                        }
                        Command<Commands::ATTACH_OBJECT_TO_CAR>(colls[colL], vehicle, doorPos.x - distX, doorPos.y - (distX - distY), doorPos.z - distX, 0.0f, 0.0f, 20.0f);
                    }
                } else if (Command<Commands::DOES_OBJECT_EXIST>(colls[colL])) {
                    Command<Commands::DELETE_OBJECT>(colls[colL]);
                }
                
                

                RwFrame *Door_rf_dummy = rpClumpFindFrameById(veh, 8);
                std::string colR = "R" + std::to_string(veh->m_nModelIndex);
                if (Door_rf_dummy && Command<Commands::IS_CAR_DOOR_FULLY_OPEN>(vehicle, 3)) {
                    CVector doorPos = localComponentOffsetPos(Door_rf_dummy);
                    if (doorPos.x != 0 || doorPos.y != 0 || doorPos.z != 0) {
                        if (!Command<Commands::DOES_OBJECT_EXIST>(colls[colR])) {
                            colls[colR] = newObject(vehicle);
                        }
                        Command<Commands::ATTACH_OBJECT_TO_CAR>(colls[colR], vehicle, doorPos.x + distX, doorPos.y - (distX - distY), doorPos.z - distX, 0.0f, 0.0f, -20.0f);
                    }
                } else if (Command<Commands::DOES_OBJECT_EXIST>(colls[colR])) {
                    Command<Commands::DELETE_OBJECT>(colls[colR]);
                }
            }
       // }
        if (CTimer::GetIsCodePaused()) {
            clearObjects();
        } else {
            removeObjectsByVehicle();
        }
}


void Door(CVehicle *vehicle)
{
    CPed *player = FindPlayerPed();
    float minDistance = 5.0f;
    if(vehicle)
        {
            float distance = (vehicle->GetPosition() - player->GetPosition()).Magnitude();
            if(distance < minDistance)
            {
                DoorCollision(vehicle);
            }
         }
}

DECL_HOOKv(VehicleRender, CVehicle *vehicle)
{
    Door(vehicle);
    VehicleRender(vehicle);
}

extern "C" void OnModLoad()
{
    HOOK(VehicleRender, libs.GetSym("_ZN8CVehicle6RenderEv"));
}
