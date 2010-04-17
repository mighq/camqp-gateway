BEGIN TRANSACTION;

-- instances
INSERT INTO "instance"	("instance_pk", "instance_id", "description")	VALUES (1, 0, "abstract instance");

-- groups
INSERT INTO "group"		("group_pk", "name")	VALUES (1, "core");

-- options
INSERT INTO "option"	("option_pk", "name")	VALUES (1, "queue_module");

-- individual settings
INSERT INTO "setting" (
	"setting_pk",
	"instance_id", "group_pk", "option_pk",
	"data_text"
) VALUES (
	1,
	0, 1, 1,
	'memory'
);

COMMIT;
