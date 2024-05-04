#include <GCIPModuleManager.h>
#include "src/GCIPHooks.h"

namespace {
    void SetupLog() {
        auto logsFolder = SKSE::log::log_directory();
        if (!logsFolder) SKSE::stl::report_and_fail("SKSE log_directory not provided, logs disabled.");
        auto pluginName = SKSE::PluginDeclaration::GetSingleton()->GetName();
        auto logFilePath = *logsFolder / std::format("{}.log", pluginName);
        auto fileLoggerPtr = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFilePath.string(), true);
        auto loggerPtr = std::make_shared<spdlog::logger>("log", std::move(fileLoggerPtr));
        spdlog::set_default_logger(std::move(loggerPtr));
        spdlog::set_level(spdlog::level::trace);
        spdlog::flush_on(spdlog::level::trace);
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse) {
    SKSE::Init(skse);
    SetupLog();

    GCIP::ModuleManager::GetSingleton()->load();

    if (!SKSE::GetMessagingInterface()->RegisterListener(GCIP::OnSKSEMessageReceived)) {
        SKSE::stl::report_and_fail("Unable to register message listener.");
    }

    GCIP::installHooks();
    GCIP::installEventSink();
    GCIP::installInputEventSink();
    return true;
}