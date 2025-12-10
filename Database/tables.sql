CREATE TABLE "operators" (
  "operator_id" serial,
  "operator_uid" uuid,
  "username" varchar,
  "password" varchar,
  "clearance" integer
);

CREATE TABLE "victim_templates" (
  "victim_template_id" serial,
  "victim_template_uid" uuid,
  "username" varchar,
  "password" varchar
);

CREATE TABLE "victims" (
  "victim_id" serial,
  "victim_uid" uuid,
  "internal_ip" inet,
  "external_ip" inet,
  "hostname" varchar,
  "username" varchar,
  "operating_system" varchar,
  "last_update" timestamp,
  "status" integer
);

CREATE TABLE "commands" (
  "command_id" serial,
  "command_uid" uuid,
  "prev" bigint,
  "nonce" bigint,
  "command" varchar,
  "signature" varchar,
  "status" integer
);

CREATE TABLE "logs" (
  "log_id" serial,
  "log_uid" uuid,
  "key" varchar,
  "value" varchar,
  "time" timestamp
);

CREATE TABLE "sessions" (
  "session_id" serial,
  "session_uid" uuid,
  "port" integer
);

CREATE TABLE "results" (
  "result_id" serial,
  "result_uid" uuid,
  "data" varchar
);

ALTER TABLE "results" ADD FOREIGN KEY ("result_id") REFERENCES "commands" ("command_id");

ALTER TABLE "sessions" ADD FOREIGN KEY ("session_id") REFERENCES "victims" ("victim_id");

ALTER TABLE "commands" ADD FOREIGN KEY ("command_id") REFERENCES "victims" ("victim_id");

ALTER TABLE "logs" ADD FOREIGN KEY ("log_id") REFERENCES "victims" ("victim_id");

ALTER TABLE "victims" ADD FOREIGN KEY ("victim_id") REFERENCES "victim_templates" ("victim_template_id");
