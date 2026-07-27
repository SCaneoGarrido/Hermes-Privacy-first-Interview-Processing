#ifndef INTERVIEW_CONTROLLER_H
#define INTERVIEW_CONTROLLER_H

#include "crow.h"
class InterviewController {
    public:
        crow::response handleInterviewRegistration(const crow::request& req); 
};


#endif // INTERVIEW_CONTROLLER_H