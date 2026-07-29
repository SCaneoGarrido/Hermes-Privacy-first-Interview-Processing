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
        crow::response processInterview(const crow::request& req, int id);
        crow::response deleteInterview(int id);
        // Descargan el archivo generado por el pipeline (Sprint 5/6) tal
        // cual esta en disco, con Content-Disposition: attachment.
        crow::response downloadTranscript(int id);
        crow::response downloadSummary(int id);

    private:
        InterviewService& m_service;
};


#endif // INTERVIEW_CONTROLLER_H
