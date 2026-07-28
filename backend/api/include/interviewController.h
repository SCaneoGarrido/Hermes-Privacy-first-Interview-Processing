#ifndef INTERVIEW_CONTROLLER_H
#define INTERVIEW_CONTROLLER_H

#include "crow.h"
#include "services/InterviewService.h"

class InterviewController {
    public:
        explicit InterviewController(InterviewService& service);

        crow::response handleInterviewRegistration(const crow::request& req);
        crow::response getInterviews(const crow::request& req);
        crow::response getInterview(int id);
        crow::response processInterview(int id);
        crow::response deleteInterview(int id);

    private:
        InterviewService& m_service;
};


#endif // INTERVIEW_CONTROLLER_H
