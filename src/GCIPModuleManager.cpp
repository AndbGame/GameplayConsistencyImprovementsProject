#include <GCIPModuleManager.h>
#include "../src/GCIPHooks.h"

void GCIP::OnSKSEMessageReceived(SKSE::MessagingInterface::Message* a_msg) {
    if (a_msg != nullptr)
    {
        switch(a_msg->type)
        {
            case SKSE::MessagingInterface::kDataLoaded:
                LOG("kDataLoaded");
            break;
            case SKSE::MessagingInterface::kPostLoad:
                LOG("kPostLoad");
                GCIP::Config::GetSingleton()->Update();

                GCIP::installRMHooks();
            break;
            case SKSE::MessagingInterface::kPreLoadGame:    //set reload flag, so we can prevent in papyrus calls of native function untill view get reset by invoking _reset
                LOG("kPreLoadGame");
            break;
            case SKSE::MessagingInterface::kPostLoadGame:   //for loading existing game
                [[fallthrough]];
            case SKSE::MessagingInterface::kNewGame:  // for new game
                LOG("kPostLoadGame | kNewGame");
                GCIP::ModuleManager::GetSingleton()->reset();
            break;
        }
    }
}

SINGLETONBODY(GCIP::ModuleManager)

void GCIP::ModuleManager::load() {
    this->encounterPlanner = GCIP::Modules::EncounterPlanner::GetSingleton();
    this->encounterPlanner->load();
}

void GCIP::ModuleManager::reset() {
    if (this->encounterPlanner != nullptr) {
        this->encounterPlanner->reset(); 
    }
}
