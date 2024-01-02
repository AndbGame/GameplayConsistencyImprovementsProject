#pragma once

#include <GCIPConfig.h>
#include <modules/EncounterPlanner.h>

namespace GCIP {

    void OnSKSEMessageReceived(SKSE::MessagingInterface::Message* a_msg);

    struct ModuleDefinition {
        Module* module;
        bool enabled;
    };

    class ModuleManager {
        SINGLETONHEADER(ModuleManager)
    public:
        void load();
        void reset();

    protected:
        GCIP::Modules::EncounterPlanner* encounterPlanner;
    };
}