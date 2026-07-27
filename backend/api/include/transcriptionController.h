#ifndef TRANSCRIPTION_CONTROLLER_H
#define TRANSCRIPTION_CONTROLLER_H

#include "crow.h"
#include <string>

class TranscriptionController {
    public:
        crow::response handleTranscription(const crow::request& req);  
}

#endif // TRANSCRIPTION_CONTROLLER_H3exi