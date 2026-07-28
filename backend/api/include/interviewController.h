#ifndef INTERVIEW_CONTROLLER_H
#define INTERVIEW_CONTROLLER_H

#include "crow.h"
class InterviewController {
    public:
        crow::response handleInterviewRegistration(const crow::request& req);
        crow::response getInterviews(const crow::request& req);
        crow::response getInterview(int id);
        crow::response processInterview(int id);
};


#endif // INTERVIEW_CONTROLLER_H