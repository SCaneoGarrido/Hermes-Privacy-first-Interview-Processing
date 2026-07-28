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
    created_at                          TIMESTAMP    DEFAULT CURRENT_TIMESTAMP,

    CONSTRAINT uq_interview_results_interview_id
        UNIQUE (interview_id),

    CONSTRAINT fk_interview_results_interview
        FOREIGN KEY (interview_id) REFERENCES interviews(id)
        ON DELETE CASCADE
        ON UPDATE CASCADE
) ENGINE=InnoDB;