#include <modules/EncounterPlanner.h>

#include <algorithm>

namespace {
    
    inline bool tryLock(PAPYRUSFUNCHANDLE, RE::TESForm* form, std::string lock_id, std::string action) {
        return GCIP::Modules::EncounterPlanner::GetSingleton()->tryLock(form, lock_id, action);
    }
    inline bool tryInterceptLock(PAPYRUSFUNCHANDLE, RE::TESForm* form, std::string lock_id, std::string action) {
        // TODO
        return GCIP::Modules::EncounterPlanner::GetSingleton()->tryLock(form, lock_id, action);
    }
    inline bool shareLock(PAPYRUSFUNCHANDLE, RE::TESForm* form, std::string lock_id, std::string allowed_lock_id) {
        return GCIP::Modules::EncounterPlanner::GetSingleton()->shareLock(form, lock_id, allowed_lock_id);
    }

    inline bool isLocked(PAPYRUSFUNCHANDLE, RE::TESForm* form, std::string lock_id) {
        return GCIP::Modules::EncounterPlanner::GetSingleton()->isLocked(form, lock_id);
    }

    inline bool unlock(PAPYRUSFUNCHANDLE, RE::TESForm* form, std::string lock_id) {
        return GCIP::Modules::EncounterPlanner::GetSingleton()->unlock(form, lock_id);
    }
    inline std::string getCurrentAction(PAPYRUSFUNCHANDLE, RE::TESForm* form) {
        return GCIP::Modules::EncounterPlanner::GetSingleton()->getCurrentAction(form);
    }

    // not implemented
    inline std::string getInterceptingLockId(PAPYRUSFUNCHANDLE) { return ""; }
    // not implemented
    inline bool setInterceptLockHandler(PAPYRUSFUNCHANDLE) { return false; }
    // not implemented
    inline bool addQueuedAction(PAPYRUSFUNCHANDLE, RE::TESForm* form, std::string lock_id, std::string action) {
        return false;
    }
    // not implemented
    inline bool tryInterceptLockOrQueue(PAPYRUSFUNCHANDLE, RE::TESForm* form, std::string lock_id) { return false; }

    bool RegisterPapyrusFunctions(RE::BSScript::IVirtualMachine* vm) {
        const bool loc_unhook = GCIP::Config::GetSingleton()->CFG_PAPYUNHOOK;

        #define REGISTERPAPYRUSFUNC(name, unhook) { vm->RegisterFunction(#name, "GCIP", name, unhook && loc_unhook); }

        REGISTERPAPYRUSFUNC(tryLock, false)
        REGISTERPAPYRUSFUNC(shareLock, false)
        REGISTERPAPYRUSFUNC(isLocked, false)
        REGISTERPAPYRUSFUNC(unlock, false)
        REGISTERPAPYRUSFUNC(getCurrentAction, false)

        #undef REGISTERPAPYRUSFUNC
        return true;
    }

}

SINGLETONBODY(GCIP::Modules::EncounterPlanner)

void GCIP::Modules::EncounterPlanner::load() {
    LOG("GCIP::Modules::EncounterPlanner::load");
    SKSE::GetPapyrusInterface()->Register(RegisterPapyrusFunctions);
    
}

void GCIP::Modules::EncounterPlanner::reset() {
    LOG("GCIP::Modules::EncounterPlanner::reset");
    this->spinLock();
    this->forms.clear();
    this->spinUnlock();
}

void GCIP::Modules::EncounterPlanner::dumpToLog() {
    LOG("GCIP::Modules::EncounterPlanner::dumpToLog");
    for (auto iter = this->forms.begin(); iter != this->forms.end(); ++iter) {
        LOG("\tForm: {:08X}", iter->first);
        if (iter->second->locks.empty()) {
            LOG("\t\t <EMPTY>", iter->first);
        } else {
            LOG("\t\t {}: {}", iter->second->locks.top()->lockId, iter->second->locks.top()->action);
            if (!iter->second->locks.top()->sharedLocks.empty()) {
                for (auto iter1 = iter->second->locks.top()->sharedLocks.begin();
                     iter1 != iter->second->locks.top()->sharedLocks.end(); ++iter1) {
                    LOG("\t\t\t shared: {}", iter1->c_str());
                }
            }
        }
    }
}

bool GCIP::Modules::EncounterPlanner::tryLock(RE::TESForm* form, std::string lock_id, std::string action) {
    LOG("GCIP::Modules::EncounterPlanner::tryLock");
    std::shared_ptr<LockedForm> lockedForm;

    // Check in lockedForms
    this->spinLock();
    auto lockedFormIt = this->forms.find(form->GetFormID());
    if (lockedFormIt == this->forms.end()) {
        lockedForm = std::make_shared<LockedForm>();
        this->forms.insert({form->GetFormID(), lockedForm});
    } else {
        lockedForm = lockedFormIt->second;
    }

    std::shared_ptr<Lock> lock;
    // Check LockedForm is empty
    if (lockedForm->locks.empty()) {
        lock = std::make_shared<Lock>();
        lock->lockId = lock_id;
        lockedForm->locks.push(lock);
    } else {
        lock = lockedForm->locks.top();
    }

    // Same Lock
    if (lock->lockId == lock_id) {
        if (lock->action != action) {
            lock->action = action;
        }
        this->spinUnlock();
        this->dumpToLog();
        return true;
    }
    // Shared Locks
    if (std::find(lock->sharedLocks.begin(), lock->sharedLocks.end(), lock_id) != lock->sharedLocks.end()) {
        lock = std::make_shared<Lock>();
        lock->lockId = lock_id;
        lock->action = action;
        lockedForm->locks.push(lock);
        this->spinUnlock();
        this->dumpToLog();
        return true;
    }

    this->spinUnlock();
    this->dumpToLog();
    return false;
}

bool GCIP::Modules::EncounterPlanner::shareLock(RE::TESForm* form, std::string lock_id, std::string allowed_lock_id) {
    LOG("GCIP::Modules::EncounterPlanner::shareLock");
    // Check in lockedForms
    this->spinLock();
    auto lockedFormIt = this->forms.find(form->GetFormID());
    if (lockedFormIt == this->forms.end()) {
        this->spinUnlock();
        return false;
    }

    // Check LockedForm is empty
    if (lockedFormIt->second->locks.empty()) {
        this->spinUnlock();
        return false;
    }

    // Check lockId amd sharedLocks in Lock
    if (lockedFormIt->second->locks.top()->lockId == lock_id) {
        if (std::find(lockedFormIt->second->locks.top()->sharedLocks.begin(),
                      lockedFormIt->second->locks.top()->sharedLocks.end(),
                      allowed_lock_id) == lockedFormIt->second->locks.top()->sharedLocks.end()) {
            lockedFormIt->second->locks.top()->sharedLocks.push_back(allowed_lock_id);
        }
        this->spinUnlock();
        this->dumpToLog();
        return true;
    }

    this->spinUnlock();
    return false;
}

bool GCIP::Modules::EncounterPlanner::isLocked(RE::TESForm* form, std::string lock_id) {
    LOG("GCIP::Modules::EncounterPlanner::isLocked");
    // Check in lockedForms
    this->spinLock();
    auto lockedFormIt = this->forms.find(form->GetFormID());
    if (lockedFormIt == this->forms.end()) {
        this->spinUnlock();
        return false;
    }

    // Check LockedForm is empty
    if (lockedFormIt->second->locks.empty()) {
        this->spinUnlock();
        return false;
    }

    // Check lockId amd sharedLocks in Lock
    if (lockedFormIt->second->locks.top()->lockId == lock_id ||
        std::find(lockedFormIt->second->locks.top()->sharedLocks.begin(),
                  lockedFormIt->second->locks.top()->sharedLocks.end(),
                  lock_id) != lockedFormIt->second->locks.top()->sharedLocks.end()) {
        this->spinUnlock();
        return false;
    }

    this->spinUnlock();
    this->dumpToLog();
    return true;
}

bool GCIP::Modules::EncounterPlanner::unlock(RE::TESForm* form, std::string lock_id) {
    LOG("GCIP::Modules::EncounterPlanner::unlock");
    // Check in lockedForms
    this->spinLock();
    auto lockedFormIt = this->forms.find(form->GetFormID());
    if (lockedFormIt == this->forms.end()) {
        this->spinUnlock();
        return true;
    }

    // Check LockedForm is empty
    if (lockedFormIt->second->locks.empty()) {
        this->spinUnlock();
        return true;
    }

    // Check lockId in Lock
    if (lockedFormIt->second->locks.top()->lockId == lock_id) {
        lockedFormIt->second->locks.pop();
        this->spinUnlock();
        return true;
    }

    this->spinUnlock();
    this->dumpToLog();
    return false;
}

std::string GCIP::Modules::EncounterPlanner::getCurrentAction(RE::TESForm* form) {
    LOG("GCIP::Modules::EncounterPlanner::getCurrentAction");
    // Check in lockedForms
    this->spinLock();
    auto lockedFormIt = this->forms.find(form->GetFormID());
    if (lockedFormIt == this->forms.end()) {
        this->spinUnlock();
        return {};
    }

    // Check LockedForm is empty
    if (lockedFormIt->second->locks.empty()) {
        this->spinUnlock();
        return {};
    }

    this->spinUnlock();

    return lockedFormIt->second->locks.top()->action;
}
