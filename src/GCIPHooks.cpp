#include "GCIPHooks.h"
#include <Windows.h>
#include <detours/detours.h>
#include <xbyak/xbyak.h>

namespace {
    struct Load3D {
        static RE::NiAVObject* thunk(RE::Character* a_this, bool a_arg1) {

            if (const auto npc = a_this->GetActorBase()) {
                SKSE::log::info("Load3D NPC actor base <{:08X}:{}>", npc->GetFormID(), npc->GetName());
            } else {
                SKSE::log::info("Load3D NPC <{:08X}:{}>", a_this->GetFormID(), a_this->GetName());
            }
            const auto root = func(a_this, a_arg1);

            SKSE::log::info("Load3D NPC done");

            return root;
        }

        static inline REL::Relocation<decltype(thunk)> func;

        static inline constexpr std::size_t index{0};
        static inline constexpr std::size_t size{0x6A};
    };
}

void GCIP::installHooks() {
    SKSE::log::info("Install hooks pre");

    //REL::Relocation<std::uintptr_t> vtbl{RE::Character::VTABLE[Load3D::index]};
    //Load3D::func = vtbl.write_vfunc(Load3D::size, Load3D::thunk);

    SKSE::log::info("Install hooks post");
}

void GCIP::installEventSink() {
    SKSE::log::info("Install EventSink pre");
    auto scriptEventSource = RE::ScriptEventSourceHolder::GetSingleton();
    if (scriptEventSource) {
        //scriptEventSource->AddEventSink(new EventSync::OnTESCombatEventHandler(defeatManager));
        //scriptEventSource->AddEventSink(new EventSync::OnTESHitEventHandler(defeatManager));
        //scriptEventSource->AddEventSink(new EventSync::OnTESEquipEventHandler(defeatManager));
    } else {
        SKSE::log::critical("Install EventSink failed");
    }
    SKSE::log::info("Install EventSink post");
}

void GCIP::installInputEventSink() {
    SKSE::log::info("Install Input EventSink pre");
    auto inputEvent = RE::BSInputDeviceManager::GetSingleton();
    if (inputEvent) {
        // inputEvent->AddEventSink(new EventSync::OnInputEventHandler(defeatManager));
    } else {
        SKSE::log::critical("Install Input EventSink failed");
    }
    SKSE::log::info("Install Input EventSink post");
}

// ---------------------------------------

typedef void(WINAPI* Original_MorphCache_ApplyMorphs)(void* morphCache, RE::TESObjectREFR* refr,
                                                      RE::NiAVObject* rootNode,
                                                      bool isAttaching,
                                                      bool defer);

inline Original_MorphCache_ApplyMorphs _MorphCache_ApplyMorphs;

bool VisitObjects(RE::NiAVObject* parent, std::function<bool(RE::NiAVObject*)> functor) {
    RE::NiNode* node = parent->AsNode();
    if (node) {
        if (functor(parent)) return true;

        //int _freeIdx = node->GetChildren()._freeIdx;
        //SKSE::log::info("        VisitObjects _freeIdx: {}", _freeIdx);
        for (int i = 0; i < node->GetChildren().size(); i++) {
            auto object = node->GetChildren()[i];
            if (object && object.get()) {
                //SKSE::log::info("        VisitObjects loop[{}] exist", i);
                if (VisitObjects(object.get(), functor)) return true;
            }
        }
    } else if (functor(parent))
        return true;

    return false;
}

inline void MorphCache_ApplyMorphs(void* morphCache, RE::TESObjectREFR* refr, RE::NiAVObject* rootNode,
                                                      bool isAttaching,
                                                      bool defer) {
    SKSE::log::info("RM MorphCache_ApplyMorphs {:08X}:{}", refr->GetFormID(), rootNode->name);

    if (refr) {
        for (int k = 0; k <= 1; ++k) {
            auto weightModel = refr->GetBiped(k);
            if (weightModel) {
                for (int i = 0; i < 42; ++i) {
                    auto& data = weightModel->objects[i];
                    auto& buffered_data = weightModel->bufferedObjects[i];

                    if (data.item) {
                        RE::BSFixedString partClone_name;
                        if (data.partClone.get()) {
                            VisitObjects(data.partClone.get(), [&](RE::NiAVObject* object) {
                            // binary search
                            RE::NiStringExtraData* BODYTRI = object->GetExtraData<RE::NiStringExtraData>("BODYTRI");
                            // linear search
                            RE::NiStringExtraData* BODYTRI_1 = nullptr;
                            for (int j = 0; j < object->extraDataSize; ++j) {
                                if (object->extra[j]) {
                                    SKSE::log::info("       RM MorphCache_ApplyMorphs::VisitObjects extra name:{}",
                                                    object->extra[j]->name.c_str());
                                    if (object->extra[j]->name == "BODYTRI") {
                                        BODYTRI_1 = reinterpret_cast<RE::NiStringExtraData*>(object->extra[j]);
                                        auto val = BODYTRI_1->value;
                                        SKSE::log::info("       RM MorphCache_ApplyMorphs::VisitObjects BODYTRI {}",
                                                        std::string(val));
                                    }
                                }
                            }
                            if (!BODYTRI && BODYTRI_1) {
                                SKSE::log::info("       RM MorphCache_ApplyMorphs::VisitObjects binary search failed");
                            }
                                std::string tri_Path;
                                if (BODYTRI) {
                                    std::string BODYTRI_Path = "meshes\\";
                                    BODYTRI_Path += std::string(BODYTRI->value);
                                    std::transform(BODYTRI_Path.begin(), BODYTRI_Path.end(), BODYTRI_Path.begin(),
                                                   ::tolower);
                                    tri_Path = BODYTRI_Path;
                                }
                                SKSE::log::info("       RM MorphCache_ApplyMorphs::VisitObjects path {}", tri_Path);
                                return false;
                            });
                            partClone_name = data.partClone.get()->name;
                        } else {
                            partClone_name = "<nullptr>";
                        }

                        SKSE::log::info("   RM MorphCache_ApplyMorphs data {}:{} {:08X}:{}. partClone_name: {}", k, i,
                            data.item->GetFormID(), data.item->GetName(), partClone_name.c_str());
                    }
                    if (buffered_data.item) {
                        RE::BSFixedString partClone_name;
                        if (buffered_data.partClone.get()) {
                            VisitObjects(buffered_data.partClone.get(), [&](RE::NiAVObject* object) {
                                auto BODYTRI = object->GetExtraData<RE::NiStringExtraData>("BODYTRI");
                                for (int j = 0; j < object->extraDataSize; ++j) {
                                    if (object->extra[j]) {
                                        SKSE::log::info("       RM MorphCache_ApplyMorphs::VisitObjects extra name:{}",
                                                        object->extra[j]->name.c_str());
                                        if (object->extra[j]->name == "BODYTRI") {
                                            auto val =
                                                reinterpret_cast<RE::NiStringExtraData*>(object->extra[j])->value;
                                            SKSE::log::info("       RM MorphCache_ApplyMorphs::VisitObjects BODYTRI {}",
                                                            std::string(val));
                                        }
                                    }
                                }
                                std::string tri_Path;
                                if (BODYTRI) {
                                    std::string BODYTRI_Path = "meshes\\";
                                    BODYTRI_Path += std::string(BODYTRI->value);
                                    std::transform(BODYTRI_Path.begin(), BODYTRI_Path.end(), BODYTRI_Path.begin(),
                                                   ::tolower);
                                    tri_Path = BODYTRI_Path;
                                }
                                SKSE::log::info("       RM MorphCache_ApplyMorphs::VisitObjects path {}  buffered_data", tri_Path);
                                return false;
                            });
                            partClone_name = buffered_data.partClone.get()->name;
                        } else {
                            partClone_name = "<nullptr>";
                        }
                        SKSE::log::info("   RM MorphCache_ApplyMorphs buffered_data {}:{} {:08X}:{}. partClone_name: {}", k, i, buffered_data.item->GetFormID(), buffered_data.item->GetName(),
                            partClone_name.c_str());
                    }
                }
            }
        }
    }

    _MorphCache_ApplyMorphs(morphCache, refr, rootNode, isAttaching, defer);
}

// ---------------------------------------

typedef void(WINAPI* Original_MorphFileCache_ApplyMorphs)(void* morphFileCache, RE::TESObjectREFR* refr,
                                                          RE::NiAVObject* rootNode, bool isAttaching, bool defer);

inline Original_MorphFileCache_ApplyMorphs _MorphFileCache_ApplyMorphs;

inline void MorphFileCache_ApplyMorphs(void* morphFileCache, RE::TESObjectREFR* refr, RE::NiAVObject* rootNode,
                                       bool isAttaching, bool defer) {
    SKSE::log::info("RM MorphFileCache_ApplyMorphs {:08X}:{}", refr->GetFormID(), rootNode->name);

    _MorphFileCache_ApplyMorphs(morphFileCache, refr, rootNode, isAttaching, defer);
}

// ---------------------------------------

typedef void(WINAPI* Original_NiObject_CreateDeepCopy)(RE::NiPointer<RE::NiObject>& a_object, void* a2, void* a3,
                                                       void* a4, void* a5, void* a6, void* a7, void* a8);

inline Original_NiObject_CreateDeepCopy _NiObject_CreateDeepCopy;

inline void NiObject_CreateDeepCopy(RE::NiPointer<RE::NiObject>& a_object, void* a2 = nullptr, void* a3 = nullptr,
                                    void* a4 = nullptr, void* a5 = nullptr, void* a6 = nullptr, void* a7 = nullptr,
                                    void* a8 = nullptr) {
    if (a_object.get()) {
        SKSE::log::info("RE NiObject_CreateDeepCopy 1 true");
    } else {
        SKSE::log::info("RE NiObject_CreateDeepCopy 1 false");
    }
    _NiObject_CreateDeepCopy(a_object, a2, a3, a4, a5, a6, a7, a8);
    if (a_object.get()) {
        SKSE::log::info("RE NiObject_CreateDeepCopy 2 true");
    } else {
        SKSE::log::info("RE NiObject_CreateDeepCopy 2 false");
    }
}

// ---------------------------------------

namespace utils {
    class ScopedCriticalSection {
    public:
        ScopedCriticalSection(LPCRITICAL_SECTION cs) : m_cs(cs) { EnterCriticalSection(m_cs); };
        ~ScopedCriticalSection() { LeaveCriticalSection(m_cs); }

    private:
        LPCRITICAL_SECTION m_cs;
    };

    inline void hash_combine(std::size_t& seed) {}

    template <typename T, typename... Rest>
    inline void hash_combine(std::size_t& seed, const T& v, Rest... rest) {
        std::hash<T> hasher;
        seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        hash_combine(seed, rest...);
    }

    size_t hash_lower(const char* str, size_t count);

    std::string format(const char* format, ...);

#if __cplusplus > 201703L
    template <typename T = std::mutex>
    using scoped_lock = std::scoped_lock<T>;
#else
    template <typename T = std::mutex>
    using scoped_lock = std::lock_guard<T>;
#endif
}
struct StringCache_Ref {
    const char* data;
    const char* c_str() const { return operator const char*(); }
    const char* Get() const { return c_str(); }
    operator const char*() const { return data ? data : ""; }
};
typedef StringCache_Ref BSFixedString;

class SKEEFixedString {
public:

    SKEEFixedString() : m_internal() { m_hash = utils::hash_lower(m_internal.c_str(), m_internal.size()); }
    SKEEFixedString(const char* str) : m_internal(str) {
        m_hash = utils::hash_lower(m_internal.c_str(), m_internal.size());
    }
    SKEEFixedString(const std::string& str) : m_internal(str) {
        m_hash = utils::hash_lower(m_internal.c_str(), m_internal.size());
    }
    SKEEFixedString(const BSFixedString& str) : m_internal(str.c_str()) {
        m_hash = utils::hash_lower(m_internal.c_str(), m_internal.size());
    }

    bool operator<(const SKEEFixedString& x) const { return _stricmp(m_internal.c_str(), x.m_internal.c_str()) < 0; }

    bool operator==(const SKEEFixedString& x) const {
        if (m_internal.size() != x.m_internal.size()) return false;

        if (_stricmp(m_internal.c_str(), x.m_internal.c_str()) == 0) return true;

        return false;
    }

    size_t length() const { return m_internal.size(); }

    std::string AsString() const { return m_internal; }
    operator BSFixedString() const { return BSFixedString(m_internal.c_str()); }
    BSFixedString AsBSFixedString() const { return operator BSFixedString(); }

    const char* c_str() const { return operator const char*(); }
    operator const char*() const { return m_internal.c_str(); }

    size_t GetHash() const { return m_hash; }

public:
    std::string m_internal;
    size_t m_hash;
};

class MorphFileCache {};


typedef bool(WINAPI* Original_MorphCache_ApplyMorphs_lambda_1)(RE::NiAVObject* object, MorphFileCache* a2);
typedef void(WINAPI* Original_MorphCache_CreateTRIPath)(void* morphCache, SKEEFixedString* result,
                                                        const char* relativePath);
typedef bool(WINAPI* Original_MorphCache_CacheFile)(void* morphCache, const char* relativePath);

inline Original_MorphCache_ApplyMorphs_lambda_1 _MorphCache_ApplyMorphs_lambda_1;
inline Original_MorphCache_CreateTRIPath _MorphCache_CreateTRIPath;
inline Original_MorphCache_CacheFile _MorphCache_CacheFile;
std::unordered_map<SKEEFixedString, MorphFileCache> _Original_MorphCache_m_data;

inline bool MorphCache_ApplyMorphs_lambda_1(RE::NiAVObject* object, void* a2) {

    RE::NiStringExtraData* BODYTRI_1 = object->GetExtraData<RE::NiStringExtraData>("BODYTRI");
    RE::NiStringExtraData* BODYTRI_2 = nullptr;

    for (int j = 0; j < object->extraDataSize; ++j) {
        if (object->extra[j]) {
            if (object->extra[j]->name == "BODYTRI") {
                BODYTRI_2 = reinterpret_cast<RE::NiStringExtraData*>(object->extra[j]);
            }
        }
    }
    if (!BODYTRI_1 && BODYTRI_2) {
        SKSE::log::info("BODYTRI_2 founded");
    }

    if (BODYTRI_2) {
        void* morphCache = nullptr;// looks must be from rcx register
        SKEEFixedString filePath;
        _MorphCache_CreateTRIPath(morphCache, &filePath, BODYTRI_2->value);
        _MorphCache_CacheFile(morphCache, filePath.m_internal.c_str());
        auto it = _Original_MorphCache_m_data.find(filePath);
        if (it != _Original_MorphCache_m_data.end()) {
            a2 = &it->second;
            return true;
        }
    }

    return false;
}

   

void GCIP::installRMHooks() {
    
    std::uintptr_t _rm_base{0};
    std::uintptr_t _rm_offset_MorphCache_ApplyMorphs = 0x001CD70;
    std::uintptr_t _rm_offset_MorphFileCache_ApplyMorphs = 0x001C6F0;

    std::uintptr_t _rm_offset_MorphCache__ApplyMorphs____2____lambda_1_ = 0x001CE30;  // 1.6.640: 0x001CE30
    void* rmHandle = nullptr;
    rmHandle = GetModuleHandle(TEXT("skee64.dll"));
    if (!rmHandle) {
        SKSE::stl::report_and_fail("skee64.dll not loaded");
    }
    _rm_base = reinterpret_cast<std::uintptr_t>(rmHandle);

    _MorphCache_ApplyMorphs = (Original_MorphCache_ApplyMorphs)(_rm_base + _rm_offset_MorphCache_ApplyMorphs);
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)_MorphCache_ApplyMorphs, (PBYTE)&MorphCache_ApplyMorphs);
    if (DetourTransactionCommit() == NO_ERROR) {
        SKSE::log::info("Installed RM hook on MorphCache_ApplyMorphs");
    } else {
        SKSE::stl::report_and_fail("Failed to install RM hook on MorphCache_ApplyMorphs");
    }

    _MorphFileCache_ApplyMorphs = (Original_MorphFileCache_ApplyMorphs)(_rm_base + _rm_offset_MorphFileCache_ApplyMorphs);
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)_MorphFileCache_ApplyMorphs, (PBYTE)&MorphFileCache_ApplyMorphs);
    if (DetourTransactionCommit() == NO_ERROR) {
        SKSE::log::info("Installed RM hook on MorphFileCache_ApplyMorphs");
    } else {
        SKSE::stl::report_and_fail("Failed to install RM hook on MorphFileCache_ApplyMorphs");
    }

    _NiObject_CreateDeepCopy = (Original_NiObject_CreateDeepCopy)REL::ID(70191).address();
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)_NiObject_CreateDeepCopy, (PBYTE)&NiObject_CreateDeepCopy);
    if (DetourTransactionCommit() == NO_ERROR) {
        SKSE::log::info("Installed RM hook on NiObject_CreateDeepCopy");
    } else {
        SKSE::stl::report_and_fail("Failed to install RM hook on NiObject_CreateDeepCopy");
    }

    
    _MorphCache_ApplyMorphs_lambda_1 =
        (Original_MorphCache_ApplyMorphs_lambda_1)(_rm_base + _rm_offset_MorphCache__ApplyMorphs____2____lambda_1_);
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)_MorphCache_ApplyMorphs_lambda_1, (PBYTE)&MorphCache_ApplyMorphs_lambda_1);
    if (DetourTransactionCommit() == NO_ERROR) {
        SKSE::log::info("Installed RM hook on MorphCache_ApplyMorphs_lambda_1");
    } else {
        SKSE::stl::report_and_fail("Failed to install RM hook on MorphCache_ApplyMorphs_lambda_1");
    }
    /*
    origCodeSize = orig.getSize();
    struct DeleteBit6 : Xbyak::CodeGenerator {
        DeleteBit6() {
            movsx(ecx, r8w);
            movsx(edx, r10w);
            add(ecx, edx);
            sar(ecx, 1);
            movsx(rdx, cx);
            mov(rbx, ptr[r11 + rdx * 8]);
            mov(r9, ptr[rbx + 16]);
            cmp(r9, rax);
            jz(); // short loc_18001CEDF
            lea(edx, ptr[rcx - 1]);
            cmovnb(r8w, dx);
            inc(cx);
            cmp(r9, rax);
            cmovnb(cx, r10w);
            movzx(r10d, cx);
            cmp(r8w, cx);
            jge     short loc_18001CEA0
            jmp     loc_18001CF94
        }
    };
    DeleteBit6 db6;
    db6.ready();
    */
}
