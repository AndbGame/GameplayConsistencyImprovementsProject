#include "GCIPConfig.h"

SINGLETONBODY(GCIP::Config)

void GCIP::Config::Update() {
    _config = boost::property_tree::ptree();
    try
    {
        boost::property_tree::ini_parser::read_ini("Data\\skse\\plugins\\GCIP.ini", _config);
    }
    catch( std::exception &ex )
    {
        ERROR("ERROR LOADING ini FILE: {}",ex.what())
        return;
    }

    CFG_PAPYUNHOOK  = _config.get<int>("General.UnhookPapyrus");
    CFG_LOGGING     = _config.get<int>("General.Logging");
    LOG("CFG_PAPYUNHOOK = {}",CFG_PAPYUNHOOK)
    LOG("CFG_LOGGING = {}",CFG_LOGGING)
    _installed = true;
}
