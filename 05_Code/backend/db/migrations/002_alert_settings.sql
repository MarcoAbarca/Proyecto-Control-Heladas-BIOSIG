CREATE TABLE IF NOT EXISTS node_alert_settings (
    node_id SMALLINT PRIMARY KEY REFERENCES nodes(node_id) ON DELETE CASCADE,
    frost_enabled BOOLEAN NOT NULL DEFAULT TRUE,
    frost_threshold_c NUMERIC(5, 2) NOT NULL DEFAULT 0.00
        CHECK (frost_threshold_c BETWEEN -80 AND 80),
    heat_enabled BOOLEAN NOT NULL DEFAULT FALSE,
    heat_threshold_c NUMERIC(5, 2) NOT NULL DEFAULT 35.00
        CHECK (heat_threshold_c BETWEEN -80 AND 100),
    sensor_enabled BOOLEAN NOT NULL DEFAULT TRUE,
    connectivity_enabled BOOLEAN NOT NULL DEFAULT TRUE,
    telegram_enabled BOOLEAN NOT NULL DEFAULT FALSE,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);