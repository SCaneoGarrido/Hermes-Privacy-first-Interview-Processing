
use hermes;
-- 1. Desactiva la verificación de llaves foráneas
SET FOREIGN_KEY_CHECKS = 0;

-- 2. Trunca todas las tablas del circuito
TRUNCATE TABLE interviews_audio;
TRUNCATE TABLE interview_results;
TRUNCATE TABLE interviews;

-- 3. Vuelve a activar la verificación de seguridad
SET FOREIGN_KEY_CHECKS = 1;
