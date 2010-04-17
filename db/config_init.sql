BEGIN TRANSACTION;

CREATE TABLE "group"
(
	"group_pk"	INTEGER	NOT NULL,
	"name"		TEXT	NOT NULL UNIQUE,

	CONSTRAINT "cns_group_pk" PRIMARY KEY ("group_pk") ON CONFLICT ABORT
);


CREATE TABLE "setting"
(
	"setting_pk"	INTEGER	NOT NULL,
	"name"			TEXT	NOT NULL UNIQUE,

	CONSTRAINT "cns_setting_pk" PRIMARY KEY ("setting_pk") ON CONFLICT ABORT
);


CREATE TABLE "value"
(
	"value_pk"		INTEGER	NOT NULL,
	"group_pk"		INTEGER	NOT NULL,
	"setting_pk"	INTEGER	NOT NULL,
	"data_int"		INTEGER,
	"data_real"		REAL,
	"data_text"		TEXT,
	"data_bin"		BLOB,

	CONSTRAINT "cns_value_pk"	PRIMARY KEY ("value_pk") ON CONFLICT ABORT,
	CONSTRAINT "cns_value_set"	CHECK (coalesce("data_int", "data_real", "data_text", "data_bin") IS NOT NULL)

);

CREATE UNIQUE INDEX "uniq_value" ON "value" ("group_pk", "setting_pk");

COMMIT;
