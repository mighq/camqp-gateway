BEGIN TRANSACTION;

-- instance
INSERT INTO "instance" (
	"instance_pk",
	"instance_id",
	"description"
) VALUES (
	coalesce((SELECT max("instance_pk")+1 FROM "instance"), 1),
	1,
	'Reception'
);

-- settings
INSERT INTO "setting" (
	"setting_pk",
	--
	"instance_id",
	"group_pk",
	"option_pk",
	--
	"data_text"
) VALUES (
	coalesce((SELECT max("setting_pk")+1 FROM "setting"), 1),
	--
	1,
	(SELECT "group_pk" FROM "group" WHERE "name" = 'core'),
	(SELECT "option_pk" FROM "option" WHERE "name" = 'log_module'),
	--
	'file'
);

INSERT INTO "setting" (
	"setting_pk",
	--
	"instance_id",
	"group_pk",
	"option_pk",
	--
	"data_text"
) VALUES (
	coalesce((SELECT max("setting_pk")+1 FROM "setting"), 1),
	--
	1,
	(SELECT "group_pk" FROM "group" WHERE "name" = 'core'),
	(SELECT "option_pk" FROM "option" WHERE "name" = 'log_filename'),
	--
	'instance_1.log'
);

INSERT INTO "setting" (
	"setting_pk",
	--
	"instance_id",
	"group_pk",
	"option_pk",
	--
	"data_text"
) VALUES (
	coalesce((SELECT max("setting_pk")+1 FROM "setting"), 1),
	--
	1,
	(SELECT "group_pk" FROM "group" WHERE "name" = 'core'),
	(SELECT "option_pk" FROM "option" WHERE "name" = 'queue_module'),
	--
	'memory'
);

INSERT INTO "setting" (
	"setting_pk",
	--
	"instance_id",
	"group_pk",
	"option_pk",
	--
	"data_text"
) VALUES (
	coalesce((SELECT max("setting_pk")+1 FROM "setting"), 1),
	--
	1,
	(SELECT "group_pk" FROM "group" WHERE "name" = 'core'),
	(SELECT "option_pk" FROM "option" WHERE "name" = 'trash_module'),
	--
	'sqlite'
);

INSERT INTO "setting" (
	"setting_pk",
	--
	"instance_id",
	"group_pk",
	"option_pk",
	--
	"data_text"
) VALUES (
	coalesce((SELECT max("setting_pk")+1 FROM "setting"), 1),
	--
	1,
	(SELECT "group_pk" FROM "group" WHERE "name" = 'core'),
	(SELECT "option_pk" FROM "option" WHERE "name" = 'input_module'),
	--
	'smpp'
);

INSERT INTO "setting" (
	"setting_pk",
	--
	"instance_id",
	"group_pk",
	"option_pk",
	--
	"data_text"
) VALUES (
	coalesce((SELECT max("setting_pk")+1 FROM "setting"), 1),
	--
	1,
	(SELECT "group_pk" FROM "group" WHERE "name" = 'core'),
	(SELECT "option_pk" FROM "option" WHERE "name" = 'output_module'),
	--
	'sqlite'
);

------------

INSERT INTO "setting" (
	"setting_pk",
	--
	"instance_id",
	"group_pk",
	"option_pk",
	--
	"data_text"
) VALUES (
	coalesce((SELECT max("setting_pk")+1 FROM "setting"), 1),
	--
	1,
	(SELECT "group_pk" FROM "group" WHERE "name" = 'msg_in_smpp'),
	(SELECT "option_pk" FROM "option" WHERE "name" = 'listen_ip'),
	--
	'192.168.108.2'
);

INSERT INTO "setting" (
	"setting_pk",
	--
	"instance_id",
	"group_pk",
	"option_pk",
	--
	"data_int"
) VALUES (
	coalesce((SELECT max("setting_pk")+1 FROM "setting"), 1),
	--
	1,
	(SELECT "group_pk" FROM "group" WHERE "name" = 'msg_in_smpp'),
	(SELECT "option_pk" FROM "option" WHERE "name" = 'listen_port'),
	--
	12345
);

COMMIT;
