CREATE TABLE IF NOT EXISTS nodes (
    node_id SMALLINT PRIMARY KEY CHECK (node_id BETWEEN 0 AND 255),
    display_name TEXT NOT NULL,
    enabled BOOLEAN NOT NULL DEFAULT TRUE,
    communication_timeout_seconds INTEGER NOT NULL DEFAULT 900
        CHECK (communication_timeout_seconds > 0),
    created_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS telemetry_measurements (
    id BIGSERIAL PRIMARY KEY,
    node_id SMALLINT NOT NULL REFERENCES nodes(node_id),
    sequence_number BIGINT NOT NULL CHECK (sequence_number BETWEEN 0 AND 4294967295),
    received_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    ttn_received_at TIMESTAMPTZ,
    temp_canopy_top_c NUMERIC(7, 2),
    temp_canopy_mid_c NUMERIC(7, 2),
    temp_canopy_low_c NUMERIC(7, 2),
    temp_base_c NUMERIC(7, 2),
    hum_base_pct NUMERIC(6, 2),
    pressure_hpa NUMERIC(7, 1),
    soil_moisture_pct NUMERIC(6, 2),
    temp_soil_c NUMERIC(7, 2),
    status_flags INTEGER NOT NULL CHECK (status_flags BETWEEN 0 AND 65535),
    raw_payload BYTEA NOT NULL,
    link_metadata JSONB NOT NULL DEFAULT '{}'::jsonb,
    quality_metadata JSONB NOT NULL DEFAULT '{}'::jsonb,
    UNIQUE (node_id, sequence_number)
);

CREATE INDEX IF NOT EXISTS telemetry_measurements_node_received_idx
    ON telemetry_measurements (node_id, received_at DESC);

CREATE TABLE IF NOT EXISTS node_current_state (
    node_id SMALLINT PRIMARY KEY REFERENCES nodes(node_id),
    measurement_id BIGINT NOT NULL REFERENCES telemetry_measurements(id),
    last_sequence_number BIGINT NOT NULL,
    last_received_at TIMESTAMPTZ NOT NULL,
    connection_state TEXT NOT NULL CHECK (connection_state IN ('online', 'stale', 'unknown')),
    last_status_flags INTEGER NOT NULL CHECK (last_status_flags BETWEEN 0 AND 65535),
    quality_metadata JSONB NOT NULL DEFAULT '{}'::jsonb,
    updated_at TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE TABLE IF NOT EXISTS alert_events (
    id BIGSERIAL PRIMARY KEY,
    node_id SMALLINT NOT NULL REFERENCES nodes(node_id),
    alert_type TEXT NOT NULL CHECK (alert_type IN ('frost', 'heat', 'sensor', 'connectivity')),
    severity TEXT NOT NULL CHECK (severity IN ('warning', 'critical')),
    state TEXT NOT NULL CHECK (state IN ('active', 'resolved')),
    measurement_id BIGINT REFERENCES telemetry_measurements(id),
    message TEXT NOT NULL,
    value NUMERIC,
    started_at TIMESTAMPTZ NOT NULL DEFAULT now(),
    resolved_at TIMESTAMPTZ,
    notification_sent_at TIMESTAMPTZ,
    details JSONB NOT NULL DEFAULT '{}'::jsonb
);

CREATE INDEX IF NOT EXISTS alert_events_active_idx
    ON alert_events (node_id, alert_type, state)
    WHERE state = 'active';

CREATE INDEX IF NOT EXISTS alert_events_started_idx
    ON alert_events (started_at DESC);