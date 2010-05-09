BEGIN TRANSACTION;

-- instance
INSERT INTO "instance" (
	"instance_pk",
	"instance_id",
	"description"
) VALUES (
	coalesce((SELECT max("instance_pk")+1 FROM "instance"), 1),
	2,
	'Sending'
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
	2,
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
	2,
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
	2,
	(SELECT "group_pk" FROM "group" WHERE "name" = 'core'),
	(SELECT "option_pk" FROM "option" WHERE "name" = 'input_module'),
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
	2,
	(SELECT "group_pk" FROM "group" WHERE "name" = 'core'),
	(SELECT "option_pk" FROM "option" WHERE "name" = 'output_module'),
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
	"data_real"
) VALUES (
	coalesce((SELECT max("setting_pk")+1 FROM "setting"), 1),
	--
	2,
	(SELECT "group_pk" FROM "group" WHERE "name" = 'core'),
	(SELECT "option_pk" FROM "option" WHERE "name" = 'forward_pull_timeout'),
	--
	3000.000 -- miliseconds
);

COMMIT;
