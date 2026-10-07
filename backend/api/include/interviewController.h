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
        crow::response updateKeywords(const crow::request& req, int id);
        crow::response deleteInterview(int id);
        // Descargan el archivo generado por el pipeline (Sprint 5/6) tal
        // cual esta en disco, con Content-Disposition: attachment.
        crow::response downloadTranscript(int id);
        crow::response downloadSummary(int id);
        // Transcripcion estructurada (bloques por hablante o parrafos con
        // marca de tiempo) para la vista de lectura del frontend.
        crow::response getTranscript(int id);
        // Guarda la version editada en la vista de lectura (PUT) / la
        // descarta y vuelve a la del pipeline (DELETE .../transcript/edits).
        crow::response updateTranscript(const crow::request& req, int id);
        crow::response restoreTranscript(int id);
        // Audio normalizado para el reproductor de la vista de lectura, con
        // soporte de Range (206) para poder saltar sin descargarlo entero.
        crow::response getAudio(const crow::request& req, int id);

    private:
        InterviewService& m_service;
};


#endif // INTERVIEW_CONTROLLER_H
