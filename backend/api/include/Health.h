#ifndef HEALTH_H
#define HEALTH_H

#include "crow.h"
#include <string>

class Health {
    public:
        crow::response healthCheck(const crow::request& req);
};
#endif //HEALTH_H