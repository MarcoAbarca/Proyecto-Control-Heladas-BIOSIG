ALTER TABLE telemetry_measurements 
ADD COLUMN IF NOT EXISTS user_comment TEXT;
