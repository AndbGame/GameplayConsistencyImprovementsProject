#pragma once

#include <GCIPModule.h>
#include <GCIPSpinLock.h>

namespace GCIP::Modules {

    struct Lock {
        std::string lockId;
        std::string action;
        std::list<std::string> sharedLocks;
    };

    struct LockedForm {
        std::stack<std::shared_ptr<Lock>> locks;
    };

    class EncounterPlanner : public Module, public SpinLock {
        SINGLETONHEADER(EncounterPlanner)
    public:
        void load();
        void reset();

        void dumpToLog();

        bool tryLock(RE::TESForm* form, std::string lock_id, std::string action);
        bool shareLock(RE::TESForm* form, std::string lock_id, std::string allowed_lock_id);
        bool isLockAllowed(RE::TESForm* form, std::string lock_id);
        bool unlock(RE::TESForm* form, std::string lock_id);
        std::string getCurrentAction(RE::TESForm* form);

    protected:
        bool isLockAllowed(RE::TESForm* form);
        std::unordered_map<RE::FormID, std::shared_ptr<LockedForm>> forms;
        struct {
            RE::TESFaction* slAnimatingFaction = nullptr;
            RE::TESFaction* defeatFaction = nullptr;
        } Forms;
    };
}