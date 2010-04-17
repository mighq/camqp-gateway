BEGIN TRANSACTION;

CREATE TABLE "instance"
(
	"instance_pk"	INTEGER	NOT NULL,
	"instance_id"	INTEGER	NOT NULL UNIQUE,
	"description"	TEXT	NOT NULL,

	CONSTRAINT "cns_instance_pk" PRIMARY KEY ("instance_pk")	ON CONFLICT ABORT
);

CREATE TABLE "group"
(
	"group_pk"	INTEGER	NOT NULL,
	"name"		TEXT	NOT NULL UNIQUE,

	CONSTRAINT "cns_group_pk" PRIMARY KEY ("group_pk")			ON CONFLICT ABORT
);


CREATE TABLE "option"
(
	"option_pk"	INTEGER	NOT NULL,
	"name"		TEXT	NOT NULL UNIQUE,

	CONSTRAINT "cns_option_pk" PRIMARY KEY ("option_pk")		ON CONFLICT ABORT
);


CREATE TABLE "setting"
(
	"setting_pk"	INTEGER NOT NULL,
	"instance_id"	INTEGER NOT NULL,
	"group_pk"		INTEGER NOT NULL,
	"option_pk"		INTEGER NOT NULL,

	"data_int"		INTEGER,
	"data_real"		REAL,
	"data_text"		TEXT,
	"data_bin"		BLOB,

	CONSTRAINT "cns_setting_pk"		PRIMARY KEY ("setting_pk")	ON CONFLICT ABORT,
	CONSTRAINT "cns_setting_set"	CHECK (coalesce("data_int", "data_real", "data_text", "data_bin") IS NOT NULL)
);

CREATE UNIQUE INDEX "unq_setting" ON "setting" ("instance_id", "group_pk", "option_pk");

COMMIT;
