BEGIN TRANSACTION;

-- === groups

-- core
INSERT INTO "group" (
	"group_pk",
	"name"
) VALUES (
	coalesce((SELECT max("group_pk")+1 FROM "group"), 1),
	"core"
);

-- === options

-- core.queue_module
INSERT INTO "option" (
	"option_pk",
	"name"
) VALUES (
	coalesce((SELECT max("option_pk")+1 FROM "option"), 1),
	"queue_module"
);

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

-- core.forward_pull_timeout
INSERT INTO "option" (
	"option_pk",
	"name"
) VALUES (
	coalesce((SELECT max("option_pk")+1 FROM "option"), 1),
	'forward_pull_timeout'
);

-- core.feedback_pull_timeout
INSERT INTO "option" (
	"option_pk",
	"name"
) VALUES (
	coalesce((SELECT max("option_pk")+1 FROM "option"), 1),
	'feedback_pull_timeout'
);

COMMIT;
