#pragma once

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

//we are not making nasa sw, so stfu
#pragma warning( disable : 4100 ) 
#pragma warning( disable : 4244 )

using namespace std::literals;

#define PAPYRUSFUNCHANDLE RE::StaticFunctionTag*

#define ROUND(x) std::lround(x)

#include <spdlog/sinks/basic_file_sink.h>
#define LOG(...)                                                                            \
    {                                                                                       \
        if (GCIP::Config::GetSingleton()->CFG_LOGGING >= 2) SKSE::log::info(__VA_ARGS__); \
    }
#define ERROR(...)                                                                           \
    {                                                                                        \
        if (GCIP::Config::GetSingleton()->CFG_LOGGING >= 1) SKSE::log::error(__VA_ARGS__); \
    }

#define SINGLETONHEADER(cname)                          \
        public:                                         \
            cname(cname &) = delete;                    \
            void operator=(const cname &) = delete;     \
            static cname* GetSingleton();               \
        protected:                                      \
            cname(){}                                   \
            ~cname(){}                                  \
            static cname* _this;

#define SINGLETONBODY(cname)                            \
        cname * cname::_this = new cname;               \
        cname * cname::GetSingleton(){return _this;}

