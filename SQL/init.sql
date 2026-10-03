-- ============================================================
-- Base de datos: hermes
-- Relaciones:
--   interviews (1) --- (1) interviews_audio
--   interviews (1) --- (1) interview_results
--
-- El "1 a 1" se fuerza poniendo UNIQUE sobre interview_id
-- en las tablas hijas: así una entrevista no puede tener
-- más de un audio ni más de un resultado asociado.
-- Version 1.0.0
-- ============================================================
USE hermes;
-- ------------------------------------------------------------
-- Tabla principal: interviews
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS interviews (
    id                      INT AUTO_INCREMENT PRIMARY KEY,
    interview_date          DATETIME     NOT NULL,
    interview_type          VARCHAR(100) NOT NULL,
    interview_subject_type  VARCHAR(100) NOT NULL,
    -- pending_audio | pending_processing | processing | completed | failed
    status                  VARCHAR(20)  NOT NULL DEFAULT 'pending_audio',
    created_at              TIMESTAMP    DEFAULT CURRENT_TIMESTAMP,
    updated_at              TIMESTAMP    DEFAULT CURRENT_TIMESTAMP
                                          ON UPDATE CURRENT_TIMESTAMP
) ENGINE=InnoDB;

-- La CREATE TABLE de arriba es idempotente pero no retroactiva: si la tabla
-- ya existia de una migracion anterior (sin "status"), esto la actualiza.
-- MySQL (a diferencia de MariaDB) no soporta "ADD COLUMN IF NOT EXISTS", asi
-- que dependemos de que DatabaseManager::migrateTables tolere el error
-- "Duplicate column name" (1060) cuando la columna ya existe.
ALTER TABLE interviews
    ADD COLUMN status VARCHAR(20) NOT NULL DEFAULT 'pending_audio'
    AFTER interview_subject_type;

-- Glosario de palabras clave de la entrevista, un termino por linea (ADR-018).
-- Idempotente: ver comentario de ALTER TABLE interviews mas arriba.
ALTER TABLE interviews
    ADD COLUMN keywords TEXT NULL
    AFTER status;

-- ------------------------------------------------------------
-- Tabla: interviews_audio
-- Referencia el audio de una entrevista (1 audio por entrevista)
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS interviews_audio (
    id                       INT AUTO_INCREMENT PRIMARY KEY,
    interview_audio_path     VARCHAR(500) NOT NULL,
    interview_audio_format   VARCHAR(20)  NOT NULL,
    interview_audio_size     BIGINT       NOT NULL, -- tamaño en bytes
    interview_id             INT          NOT NULL,
    created_at               TIMESTAMP    DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT uq_interviews_audio_interview_id
        UNIQUE (interview_id),

    CONSTRAINT fk_interviews_audio_interview
        FOREIGN KEY (interview_id) REFERENCES interviews(id)
        ON DELETE CASCADE
        ON UPDATE CASCADE
) ENGINE=InnoDB;

-- ------------------------------------------------------------
-- Tabla: interview_results
-- Referencia la transcripción/resultado de una entrevista
-- (1 resultado por entrevista)
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS interview_results (
    id                                  INT AUTO_INCREMENT PRIMARY KEY,
    interview_transcription_file_path   VARCHAR(500) NOT NULL,
    interview_id                        INT          NOT NULL,
    -- Ruta al resumen final (Fase 3, Sprint 6). NULL si Ollama no corrio o
    -- fallo esa fase - la transcripcion sigue siendo valida sin resumen.
    summary_file_path                   VARCHAR(500) NULL,
    created_at                          TIMESTAMP    DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT uq_interview_results_interview_id
        UNIQUE (interview_id),

    CONSTRAINT fk_interview_results_interview
        FOREIGN KEY (interview_id) REFERENCES interviews(id)
        ON DELETE CASCADE
        ON UPDATE CASCADE
) ENGINE=InnoDB;

-- Idempotente: ver comentario de ALTER TABLE interviews mas arriba.
ALTER TABLE interview_results
    ADD COLUMN summary_file_path VARCHAR(500) NULL
    AFTER interview_transcription_file_path;

-- ------------------------------------------------------------
-- Tabla: interview_jobs (Sprint 4 - Background Processing)
-- Un intento de procesamiento (Whisper/Ollama) por fila. A diferencia de
-- interviews_audio / interview_results, interview_id NO es UNIQUE aca:
-- una entrevista puede reintentarse tras un failed, y eso crea una fila
-- nueva (no un update de la vieja), sirviendo de historial/auditoria.
-- interviews.status sigue siendo el estado "grueso" que consume el
-- frontend: esta tabla es el detalle fino por intento.
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS interview_jobs (
    id                    INT AUTO_INCREMENT PRIMARY KEY,
    interview_id          INT          NOT NULL,
    -- pending | running | completed | failed
    status                VARCHAR(20)  NOT NULL DEFAULT 'pending',
    error_message         VARCHAR(500) NULL,
    -- Ruta al JSON crudo de whisper.cpp (segmentos + timestamps), Sprint 5.
    -- Es la entrada que Sprint 6 (Ollama) va a leer para la diarizacion.
    raw_transcript_path   VARCHAR(500) NULL,
    -- Paso grueso actual, solo mientras status='running' (se limpia a NULL
    -- al completar/fallar): normalizando_audio | transcribiendo |
    -- corrigiendo_texto | anonimizando | generando_resumen. Feedback de UI
    -- para que un job de varios minutos no parezca trabado.
    current_step          VARCHAR(50)  NULL,
    started_at            DATETIME     NULL,
    finished_at           DATETIME     NULL,
    created_at            TIMESTAMP    DEFAULT CURRENT_TIMESTAMP,
    updated_at            TIMESTAMP    DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,

    CONSTRAINT fk_interview_jobs_interview
        FOREIGN KEY (interview_id) REFERENCES interviews(id)
        ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB;

-- Idempotente: ver comentario de ALTER TABLE interviews mas arriba sobre por
-- que DatabaseManager::migrateTables tolera el error 1060 (columna ya existe).
ALTER TABLE interview_jobs
    ADD COLUMN raw_transcript_path VARCHAR(500) NULL
    AFTER error_message;

ALTER TABLE interview_jobs
    ADD COLUMN current_step VARCHAR(50) NULL
    AFTER raw_transcript_path;