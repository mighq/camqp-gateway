BEGIN TRANSACTION;

-- === instances
INSERT INTO "instance" (
	"instance_pk",
	"instance_id",
	"description"
) VALUES (
	coalesce((SELECT max("instance_pk")+1 FROM "instance"), 1),
	1,
	'Reception'
);

-- === groups

-- core already defined

-- === options

-- core.input_module
INSERT INTO "option" (
	"option_pk",
	"name"
) VALUES (
	coalesce((SELECT max("option_pk")+1 FROM "option"), 1),
	'input_module'
);

-- core.output_module
INSERT INTO "option" (
	"option_pk",
	"name"
) VALUES (
	coalesce((SELECT max("option_pk")+1 FROM "option"), 1),
	'output_module'
);

-- core.trash_module
INSERT INTO "option" (
	"option_pk",
	"name"
) VALUES (
	coalesce((SELECT max("option_pk")+1 FROM "option"), 1),
	'trash_module'
);

-- === individual settings
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

COMMIT;
