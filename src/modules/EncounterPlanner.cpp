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

    inline bool isLockAllowed(PAPYRUSFUNCHANDLE, RE::TESForm* form, std::string lock_id) {
        return GCIP::Modules::EncounterPlanner::GetSingleton()->isLockAllowed(form, lock_id);
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
        REGISTERPAPYRUSFUNC(isLockAllowed, false)
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

    RE::TESDataHandler* handler = RE::TESDataHandler::GetSingleton();

#define LOAD_FORM(NAME, TYPE, ID, PLUGIN)                                                                  \
    NAME = handler->LookupForm<TYPE>(ID, PLUGIN);                                                          \
    if (NAME == nullptr) {                                                                                 \
        SKSE::log::error("initializeForms : Not found <" #TYPE " - " #ID "> '" #NAME "' in '" PLUGIN "'"); \
    }

    LOAD_FORM(Forms.slAnimatingFaction, RE::TESFaction, 0x00E50F, "SexLab.esm");
    LOAD_FORM(Forms.defeatFaction, RE::TESFaction, 0x001D92, "SexLabDefeat.esp");

#undef LOAD_FORM
}

void GCIP::Modules::EncounterPlanner::dumpToLog() {
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
    if (!isLockAllowed(form)) {
        return false;
    }

    LOG("GCIP::Modules::EncounterPlanner::tryLock {:08X}:{}", form->GetFormID(), lock_id);
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
        LOG("GCIP::Modules::EncounterPlanner::tryLock - Locked {:08X}:{}", form->GetFormID(), lock_id);
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
        LOG("GCIP::Modules::EncounterPlanner::tryLock - Locked by sharedLocks {:08X}:{}", form->GetFormID(), lock_id);
        return true;
    }

    this->spinUnlock();
    this->dumpToLog();
    LOG("GCIP::Modules::EncounterPlanner::tryLock - Not Locked {:08X}:{}", form->GetFormID(), lock_id);
    return false;
}

bool GCIP::Modules::EncounterPlanner::shareLock(RE::TESForm* form, std::string lock_id, std::string allowed_lock_id) {
    LOG("GCIP::Modules::EncounterPlanner::shareLock {:08X}:{} - {}", form->GetFormID(), lock_id, allowed_lock_id);
    // Check in lockedForms
    this->spinLock();
    auto lockedFormIt = this->forms.find(form->GetFormID());
    if (lockedFormIt == this->forms.end()) {
        this->spinUnlock();
        this->dumpToLog();
        LOG("GCIP::Modules::EncounterPlanner::shareLock - Not Locked {:08X}:{} - {}", form->GetFormID(), lock_id,
            allowed_lock_id);
        return false;
    }

    // Check LockedForm is empty
    if (lockedFormIt->second->locks.empty()) {
        this->spinUnlock();
        this->dumpToLog();
        LOG("GCIP::Modules::EncounterPlanner::shareLock - Not have Locks {:08X}:{} - {}", form->GetFormID(), lock_id,
            allowed_lock_id);
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
        LOG("GCIP::Modules::EncounterPlanner::shareLock - Success {:08X}:{} - {}", form->GetFormID(), lock_id,
            allowed_lock_id);
        return true;
    }

    this->spinUnlock();
    this->dumpToLog();
    LOG("GCIP::Modules::EncounterPlanner::shareLock - Wrong Lock {:08X}:{} - {}", form->GetFormID(), lock_id,
        allowed_lock_id);
    return false;
}

bool GCIP::Modules::EncounterPlanner::isLockAllowed(RE::TESForm* form, std::string lock_id) {
    if (!isLockAllowed(form)) {
        return false;
    }

    LOG("GCIP::Modules::EncounterPlanner::isLockAllowed {:08X}:{}", form->GetFormID(), lock_id);
    // Check in lockedForms
    this->spinLock();
    auto lockedFormIt = this->forms.find(form->GetFormID());
    if (lockedFormIt == this->forms.end()) {
        this->spinUnlock();
        this->dumpToLog();
        LOG("GCIP::Modules::EncounterPlanner::isLockAllowed - Not Locked {:08X}:{}", form->GetFormID(), lock_id);
        return true;
    }

    // Check LockedForm is empty
    if (lockedFormIt->second->locks.empty()) {
        this->spinUnlock();
        this->dumpToLog();
        LOG("GCIP::Modules::EncounterPlanner::isLockAllowed - Not have Locks {:08X}:{}", form->GetFormID(), lock_id);
        return true;
    }

    // Check lockId amd sharedLocks in Lock
    if (lockedFormIt->second->locks.top()->lockId == lock_id ||
        std::find(lockedFormIt->second->locks.top()->sharedLocks.begin(),
                  lockedFormIt->second->locks.top()->sharedLocks.end(),
                  lock_id) != lockedFormIt->second->locks.top()->sharedLocks.end()) {
        this->spinUnlock();
        this->dumpToLog();
        LOG("GCIP::Modules::EncounterPlanner::isLockAllowed - Lock allowed by lock_id or sharedLocks {:08X}:{}",
            form->GetFormID(), lock_id);
        return true;
    }

    this->spinUnlock();
    this->dumpToLog();
    LOG("GCIP::Modules::EncounterPlanner::isLockAllowed - Locked {:08X}:{}", form->GetFormID(), lock_id);
    return false;
}

bool GCIP::Modules::EncounterPlanner::unlock(RE::TESForm* form, std::string lock_id) {
    LOG("GCIP::Modules::EncounterPlanner::unlock {:08X}:{}", form->GetFormID(), lock_id);
    // Check in lockedForms
    this->spinLock();
    auto lockedFormIt = this->forms.find(form->GetFormID());
    if (lockedFormIt == this->forms.end()) {
        this->spinUnlock();
        this->dumpToLog();
        LOG("GCIP::Modules::EncounterPlanner::unlock - Not locked {:08X}:{}", form->GetFormID(), lock_id);
        return true;
    }

    // Check LockedForm is empty
    if (lockedFormIt->second->locks.empty()) {
        this->spinUnlock();
        this->dumpToLog();
        LOG("GCIP::Modules::EncounterPlanner::unlock - Not have locks {:08X}:{}", form->GetFormID(), lock_id);
        return true;
    }

    // Check lockId in Lock
    if (lockedFormIt->second->locks.top()->lockId == lock_id) {
        lockedFormIt->second->locks.pop();
        this->spinUnlock();
        this->dumpToLog();
        LOG("GCIP::Modules::EncounterPlanner::unlock - Unlocked {:08X}:{}", form->GetFormID(), lock_id);
        return true;
    }

    this->spinUnlock();
    this->dumpToLog();
    LOG("GCIP::Modules::EncounterPlanner::unlock - Locked by another lock {:08X}:{}", form->GetFormID(), lock_id);
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

bool GCIP::Modules::EncounterPlanner::isLockAllowed(RE::TESForm* form) {
    // Base Mods
    auto actor = form->As<RE::Actor>();
    if (actor != nullptr) {
        // SexLab
        if (Forms.slAnimatingFaction != nullptr && actor->IsInFaction(Forms.slAnimatingFaction)) {
            LOG("GCIP::Modules::EncounterPlanner::isLockAllowed - not allowed {:08X} by {}", form->GetFormID(),
                "slAnimatingFaction");
            return false;
        }
        // Defeat
        if (Forms.defeatFaction != nullptr && actor->IsInFaction(Forms.defeatFaction)) {
            LOG("GCIP::Modules::EncounterPlanner::isLockAllowed - not allowed {:08X} by {}", form->GetFormID(), "defeatFaction");
            return false;
        }
    }
    return true;
}
