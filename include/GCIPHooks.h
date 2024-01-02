#pragma once

#include <GCIPConfig.h>

namespace GCIP {

    class Hooks {
        public:
        static void installHooks() {
            LOG("Install hooks pre")

            LOG("Install hooks post")
        }

        private:
    };
}